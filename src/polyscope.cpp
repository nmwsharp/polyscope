#include "polyscope/polyscope.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "imgui.h"
#include "polyscope/imgui_render.h"

#include "polyscope/pick.h"
#include "polyscope/view.h"

#include "json/json.hpp"
using json = nlohmann::json;


using std::cout;
using std::endl;

namespace polyscope {

// === Declare storage global members

namespace state {

bool initialized = false;

double lengthScale = 1.0;
std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> boundingBox;
Vector3 center{0, 0, 0};

std::map<std::string, std::map<std::string, Structure*>> structures;

std::function<void()> userCallback;
size_t screenshotInd = 0;

} // namespace state

std::function<void()> focusedPopupUI;

namespace options {

std::string programName = "Polyscope";
int verbosity = 2;
std::string printPrefix = "Polyscope: ";
bool errorsThrowExceptions = false;
bool debugDrawPickBuffer = false;
int maxFPS = 60;
bool usePrefsFile = true;
bool initializeWithDefaultStructures = true;
bool autocenterStructures = false;

} // namespace options

// Small callback function for GLFW errors
void error_print_callback(int error, const char* description) {
  std::cerr << "GLFW emitted error: " << description << std::endl;
}

// Forward declare compressed binary font functions
unsigned int getCousineRegularCompressedSize();
const unsigned int* getCousineRegularCompressedData();

// Helpers
namespace {

// Pick buffer state
GLuint pickFramebuffer, rboPickDepth, rboPickColor, currPickBufferWidth, currPickBufferHeight;

// Font atlas pointer
ImFontAtlas* globalFontAtlas = nullptr;

void allocatePickRenderbuffers() {

  glGenRenderbuffers(1, &rboPickDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, rboPickDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, view::bufferWidth, view::bufferHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboPickDepth);

  glGenRenderbuffers(1, &rboPickColor);
  glBindRenderbuffer(GL_RENDERBUFFER, rboPickColor);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, view::bufferWidth, view::bufferHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboPickColor);

  currPickBufferWidth = view::bufferWidth;
  currPickBufferHeight = view::bufferHeight;
}

void initPickBuffer() {

  // Create the new buffer
  glGenFramebuffers(1, &pickFramebuffer);

  // Bind to the new buffer
  glBindFramebuffer(GL_FRAMEBUFFER, pickFramebuffer);

  allocatePickRenderbuffers();
}

void setStyle() {

  // Style
  ImGuiStyle* style = &ImGui::GetStyle();
  style->WindowRounding = 1;
  style->FrameRounding = 1;
  style->FramePadding.y = 4;
  style->ScrollbarRounding = 1;
  style->ScrollbarSize = 20;


  // Colors
  ImVec4* colors = style->Colors;
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
  colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.63f, 0.63f, 0.63f, 0.39f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.69f, 0.59f, 0.40f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.41f, 0.64f, 0.53f, 0.69f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.54f, 0.42f, 0.83f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.63f, 0.49f, 0.87f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.80f, 0.62f, 0.20f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.55f, 0.48f, 0.80f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.63f, 0.63f, 0.63f, 0.39f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.80f, 0.62f, 0.40f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
  colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
  colors[ImGuiCol_Button] = ImVec4(0.35f, 0.61f, 0.49f, 0.62f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.71f, 0.57f, 0.79f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.80f, 0.64f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.40f, 0.90f, 0.67f, 0.45f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.90f, 0.69f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.87f, 0.71f, 0.80f);
  colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.70f, 0.66f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.90f, 0.81f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 1.00f, 0.90f, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 1.00f, 0.90f, 0.90f);
  colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
  colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
}

const std::string prefsFilename = ".polyscope.ini";

void readPrefsFile() {

  try {

    std::ifstream inStream(prefsFilename);
    if (inStream) {

      json prefsJSON;
      inStream >> prefsJSON;

      // Set values
      if (prefsJSON.count("windowWidth") > 0) {
        view::windowWidth = prefsJSON["windowWidth"];
      }
      if (prefsJSON.count("windowHeight") > 0) {
        view::windowHeight = prefsJSON["windowHeight"];
      }
      if (prefsJSON.count("windowPosX") > 0) {
        view::initWindowPosX = prefsJSON["windowPosX"];
      }
      if (prefsJSON.count("windowPosY") > 0) {
        view::initWindowPosY = prefsJSON["windowPosY"];
      }
    }

  }
  // We never really care if something goes wrong while loading preferences, so eat all exceptions
  catch (...) {
    polyscope::warning("Parsing of prefs file failed");
  }
}

void writePrefsFile() {

  // Update values as needed
  glfwGetWindowPos(imguirender::mainWindow, &view::initWindowPosX, &view::initWindowPosY);

  // Build json object
  json prefsJSON = {
      {"windowWidth", view::windowWidth},
      {"windowHeight", view::windowHeight},
      {"windowPosX", view::initWindowPosX},
      {"windowPosY", view::initWindowPosY},
  };

  // Write out json object
  std::ofstream o(prefsFilename);
  o << std::setw(4) << prefsJSON << std::endl;
}

}; // namespace

// === Core global functions

void init() {
  if (state::initialized) {
    throw std::logic_error(options::printPrefix + "Initialize called twice");
  }

  if (options::usePrefsFile) {
    readPrefsFile();
  }

  // === Initialize glfw
  glfwSetErrorCallback(error_print_callback);
  if (!glfwInit()) {
    throw std::runtime_error(options::printPrefix + "ERROR: Failed to initialize glfw");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  imguirender::mainWindow =
      glfwCreateWindow(view::windowWidth, view::windowHeight, options::programName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(imguirender::mainWindow);
  glfwSwapInterval(1); // Enable vsync
  glfwSetWindowPos(imguirender::mainWindow, view::initWindowPosX, view::initWindowPosY);

// === Initialize openGL
// Load openGL functions (using GLAD)
#ifndef __APPLE__
  if (!gladLoadGL()) {
    throw std::runtime_error(options::printPrefix + "ERROR: Failed to load openGL using GLAD");
  }
#endif
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "Loaded openGL version: " << glGetString(GL_VERSION) << std::endl;
  }

#ifdef __APPLE__
  // Hack to classify the process as interactive
  glfwPollEvents();
#endif

  // Update the width and heigh
  glfwMakeContextCurrent(imguirender::mainWindow);
  glfwGetWindowSize(imguirender::mainWindow, &view::windowWidth, &view::windowHeight);
  glfwGetFramebufferSize(imguirender::mainWindow, &view::bufferWidth, &view::bufferHeight);

  // Initialie ImGUI
  initializeImGUIContext();

  // Initialize common shaders
  gl::GLProgram::initCommonShaders();

  // Initialize pick buffer
  initPickBuffer();

  // Initialize with default maps so they show up in UI and user knows they exist
  if (options::initializeWithDefaultStructures) {
    state::structures[PointCloud::structureTypeName] = {};
    state::structures[SurfaceMesh::structureTypeName] = {};
    state::structures[CameraView::structureTypeName] = {};
    state::structures[RaySet::structureTypeName] = {};
  }

  state::initialized = true;
}


ImFontAtlas* getGlobalFontAtlas() {
  return globalFontAtlas;
}

void initializeImGUIContext() {

  ImGui::CreateContext();

  // Set up ImGUI glfw bindings
  imguirender::ImGui_ImplGlfwGL3_Init(imguirender::mainWindow, true);

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig config;
  config.OversampleH = 5;
  config.OversampleV = 5;
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF(
  //     "../deps/imgui/imgui/extra_fonts/Cousine-Regular.ttf", 15.0f, &config);
  ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(getCousineRegularCompressedData(),
                                                          getCousineRegularCompressedSize(), 15.0f, &config);
  // ImGui::StyleColorsLight();
  setStyle();

  globalFontAtlas = io.Fonts;
}

namespace {

// Keep track of whether or not the last click was a double click.
// ImGUI normally provides this for us, but we want to know about the RELEASE of a double click, while im ImGUI the flag
// is only set for the down press. Use this variable to pass it forward.
bool lastClickWasDouble = false;
}

namespace pick {

void evaluatePickQuery(int xPos, int yPos) {

  // Be sure not to pick outside of buffer
  if (xPos < 0 || xPos >= view::bufferWidth || yPos < 0 || yPos >= view::bufferHeight) {
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, pickFramebuffer);

  if ((int)currPickBufferWidth != view::bufferWidth || (int)currPickBufferHeight != view::bufferHeight) {
    // Delete the existing renderbuffers
    GLuint rBuffers[2] = {rboPickDepth, rboPickColor};
    glDeleteRenderbuffers(2, rBuffers);

    // Allocate some new one
    allocatePickRenderbuffers();
  }

  // Set the draw buffer
  GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, buffers);

  // Clear the pick buffer
  glViewport(0, 0, view::bufferWidth, view::bufferHeight);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Render pick buffer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  for (auto cat : state::structures) {
    for (auto x : cat.second) {
      x.second->drawPick();
    }
  }
  glFlush();
  glFinish();

  // Read from the pick buffer
  float result[4];
  glReadPixels(xPos, view::bufferHeight - yPos, 1, 1, GL_RGBA, GL_FLOAT, &result);
  gl::checkGLError(true);


  size_t ind = pick::vecToInd(geometrycentral::Vector3{result[0], result[1], result[2]});

  if (ind == 0) {
    pick::resetPick();
  } else {
    pick::setCurrentPickElement(ind, lastClickWasDouble);
  }
}
}

namespace {

float dragDistSinceLastRelease = 0.0;

void processMouseEvents() {
  ImGuiIO& io = ImGui::GetIO();

  bool shouldEvaluatePick = pick::alwaysEvaluatePick;
  if(pick::alwaysEvaluatePick) {
    pick::resetPick();
  }

  if (ImGui::IsMouseClicked(0)) {
    lastClickWasDouble = ImGui::IsMouseDoubleClicked(0);
  }

  if (!io.WantCaptureMouse) {

    // Handle drags
    if (ImGui::IsMouseDragging(0) &&
        !(io.KeyCtrl && !io.KeyShift)) { // if ctrl is pressed but shift is not, don't process a drag

      Vector2 dragDelta{io.MouseDelta.x / view::windowWidth, -io.MouseDelta.y / view::windowHeight};
      bool isDragZoom = io.KeyShift && io.KeyCtrl;
      bool isRotate = !io.KeyShift;
      if (isDragZoom) {
        view::processZoom(dragDelta.y * 5);
      } else {
        if (isRotate) {
          view::processRotate(dragDelta.x, dragDelta.y);


          /* Mediocre arcball
          Vector2 currPos{io.MousePos.x / view::windowWidth, (view::windowHeight - io.MousePos.y) / view::windowHeight};
          currPos = (currPos * 2.0) - Vector2{1.0, 1.0};
          if (std::abs(currPos.x) <= 1.0 && std::abs(currPos.y) <= 1.0) {
            view::processRotateArcball(currPos - 2.0 * dragDelta, currPos);
          }
          */
        } else {
          view::processTranslate(dragDelta);
        }
      }

      dragDistSinceLastRelease += std::abs(dragDelta.x);
      dragDistSinceLastRelease += std::abs(dragDelta.y);
    }
    // Handle picks
    else {

      if (!messageIsBlockingScreen() && ImGui::IsMouseReleased(0)) {

        ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
        if (dragDistSinceLastRelease < .01) {
          shouldEvaluatePick = true;
        }

        dragDistSinceLastRelease = 0.0;
      }
    }
  }

  if (shouldEvaluatePick) {
    ImVec2 p = ImGui::GetMousePos();
    pick::evaluatePickQuery(io.DisplayFramebufferScale.x * p.x, io.DisplayFramebufferScale.y * p.y);
  }
}

void drawStructures() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  for (auto catMap : state::structures) {
    for (auto s : catMap.second) {

      // Draw the pick buffer for debugging purposes
      if (options::debugDrawPickBuffer) {
        s.second->drawPick();
      }
      // The normal case
      else {
        s.second->draw();
      }
    }
  }
}

void buildPolyscopeGui() {
  // Create window
  static bool showPolyscopeWindow = true;
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

  ImGui::Begin("Polyscope", &showPolyscopeWindow, ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::ColorEdit3("background color", (float*)&view::bgColor, ImGuiColorEditFlags_NoInputs);
  if (ImGui::Button("Reset view")) {
    view::flyToDefault();
  }
  if (ImGui::Button("Screenshot")) {
    screenshot(true);
  }
  ImGui::Text("%.1f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  // cout << "fps = " << ImGui::GetIO().Framerate << endl;

  // == Debugging-related options
  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("debug")) {
    ImGui::Checkbox("Show pick buffer", &options::debugDrawPickBuffer);
    ImGui::TreePop();
  }

  ImGui::End();
}

void buildStructureGui() {
  // Create window
  static bool showStructureWindow = true;
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::Begin("Structures", &showStructureWindow);

  for (auto catMapEntry : state::structures) {
    std::string catName = catMapEntry.first;

    std::map<std::string, Structure*>& structureMap = catMapEntry.second;

    ImGui::PushID(catName.c_str()); // ensure there are no conflicts with
                                    // identically-named labels

    // Build the structure's UI
    ImGui::SetNextTreeNodeOpen(structureMap.size() > 0, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader(("Category: " + catName + " (" + std::to_string(structureMap.size()) + ")").c_str())) {
      // Draw shared GUI elements for all instances of the structure
      if (structureMap.size() > 0) {
        structureMap.begin()->second->drawSharedStructureUI();
      }

      for (auto x : structureMap) {
        ImGui::SetNextTreeNodeOpen(structureMap.size() <= 8,
                                   ImGuiCond_FirstUseEver); // closed by default if more than 8
        x.second->drawUI();
      }
    }

    ImGui::PopID();
  }

  ImGui::End();
}

void buildUserGui() {
  if (state::userCallback) {
    ImGui::PushID("user_callback");
    state::userCallback();
    ImGui::PopID();
  }
}

void buildPickGui() {
  if (pick::haveSelection) {
    ImGui::Begin("Selection", nullptr);
    size_t pickInd;
    Structure* structure = pick::getCurrentPickElement(pickInd);

    ImGui::TextUnformatted((structure->type + ": " + structure->name).c_str());
    ImGui::Separator();
    structure->drawPickUI(pickInd);

    ImGui::End();
  }
}

namespace {
auto lastMainLoopIterTime = std::chrono::steady_clock::now();
}

void draw(bool withUI = true) {

  // Update buffer and context
  glfwMakeContextCurrent(imguirender::mainWindow);
  glfwGetWindowSize(imguirender::mainWindow, &view::windowWidth, &view::windowHeight);
  glfwGetFramebufferSize(imguirender::mainWindow, &view::bufferWidth, &view::bufferHeight);

  if (withUI) {
    imguirender::ImGui_ImplGlfwGL3_NewFrame();
  }

  bindDefaultBuffer();

  // Ensure the default framebuffer is bound
  glClearColor(view::bgColor[0], view::bgColor[1], view::bgColor[2], 0);
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Build the GUI components
  if (withUI) {
    //ImGui::ShowDemoWindow();

    // The common case, rendering UI and structures
    if (!focusedPopupUI) {

      // Note: It is important to build the user GUI first, because it is likely that callbacks there will modify
      // polyscope data. If we do these modifications happen later in the render cycle, they might invalidate data which
      // is necessary when ImGui::Render() happens below.
      buildUserGui();

      buildPolyscopeGui();
      buildStructureGui();
      buildPickGui();

    }
    // If there is a popup UI active, only draw that
    else {
      focusedPopupUI();
    }


    buildMessagesUI();
  }

  // Draw structures in the scene
  drawStructures();

  // Draw the GUI
  if (withUI) {
    ImGui::Render();
    gl::checkGLError();
  }
}

} // namespace


void bindDefaultBuffer() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, view::bufferWidth, view::bufferHeight);
}

void mainLoopIteration() {

  // The windowing system will let this busy-loop in some situations, unfortunately. Make sure that doesn't happen.
  if (options::maxFPS != -1) {
    auto currTime = std::chrono::steady_clock::now();
    long microsecPerLoop = 1000000 / options::maxFPS;
    microsecPerLoop = 95 * microsecPerLoop / 100; // give a little slack so we actually hit target fps
    while (std::chrono::duration_cast<std::chrono::microseconds>(currTime - lastMainLoopIterTime).count() <
           microsecPerLoop) {
      // std::chrono::milliseconds timespan(1);
      // std::this_thread::sleep_for(timespan);
      std::this_thread::yield();
      currTime = std::chrono::steady_clock::now();
    }
  }
  lastMainLoopIterTime = std::chrono::steady_clock::now();


  // Update the width and heigh
  glfwMakeContextCurrent(imguirender::mainWindow);
  glfwGetWindowSize(imguirender::mainWindow, &view::windowWidth, &view::windowHeight);
  glfwGetFramebufferSize(imguirender::mainWindow, &view::bufferWidth, &view::bufferHeight);

  // Process UI events
  glfwPollEvents();
  processMouseEvents();

  // Rendering
  draw();
  glfwSwapBuffers(imguirender::mainWindow);
}

void show(bool shutdownAfter) {
  view::resetCameraToDefault();

  // Main loop
  while (!glfwWindowShouldClose(imguirender::mainWindow)) {
    mainLoopIteration();
  }

  if (shutdownAfter) {
    shutdown();
  }
}

void shutdown(int exitCode) {

  // TODO should we make an effort to destruct everything here?
  if (options::usePrefsFile) {
    writePrefsFile();
  }

  ImGui::DestroyContext();
  std::exit(exitCode);
}

bool registerStructure(Structure* s, bool replaceIfPresent) {

  // Make sure a map for the type exists
  if (state::structures.find(s->type) == state::structures.end()) {
    state::structures[s->type] = std::map<std::string, Structure*>();
  }
  std::map<std::string, Structure*>& sMap = state::structures[s->type];

  // Check if the structure name is in use
  bool inUse = sMap.find(s->name) != sMap.end();
  if (inUse) {
    if (replaceIfPresent) {
      removeStructure(s->name);
    } else {
      polyscope::error("Attempted to register structure with name " + s->name +
                       ", but a structure with that name already exists");
      return false;
    }
  }

  // Add the new structure
  sMap[s->name] = s;
  updateStructureExtents();

  return true;
}

void registerPointCloud(std::string name, const std::vector<Vector3>& points, bool replaceIfPresent) {
  PointCloud* s = new PointCloud(name, points);
  bool success = registerStructure(s);
  if (!success) delete s;
}

void registerSurfaceMesh(std::string name, Geometry<Euclidean>* geom, bool replaceIfPresent) {
  SurfaceMesh* s = new SurfaceMesh(name, geom);
  bool success = registerStructure(s);
  if (!success) delete s;
}

void registerCameraView(std::string name, CameraParameters p, bool replaceIfPresent) {
  CameraView* s = new CameraView(name, p);
  bool success = registerStructure(s);
  if (!success) delete s;
}

void registerRaySet(std::string name, const std::vector<std::vector<RayPoint>>& r, bool replaceIfPresent) {
  RaySet* s = new RaySet(name, r);
  bool success = registerStructure(s);
  if (!success) delete s;
}

Structure* getStructure(std::string type, std::string name) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    error("No structures of type " + type + " registered");
    return nullptr;
  }
  std::map<std::string, Structure*>& sMap = state::structures[type];

  // Special automatic case, return any
  if (name == "") {
    if (sMap.size() != 1) {
      error("Cannot use automatic structure get with empty name unless there is exactly one structure of that type "
            "registered");
      return nullptr;
    }
    return sMap.begin()->second;
  }

  // General case
  if (sMap.find(name) == sMap.end()) {
    error("No structure of type " + type + " with name " + name + " registered");
    return nullptr;
  }
  return sMap[name];
}


PointCloud* getPointCloud(std::string name) {
  return dynamic_cast<PointCloud*>(getStructure(PointCloud::structureTypeName, name));
}

SurfaceMesh* getSurfaceMesh(std::string name) {
  return dynamic_cast<SurfaceMesh*>(getStructure(SurfaceMesh::structureTypeName, name));
}

CameraView* getCameraView(std::string name) {
  return dynamic_cast<CameraView*>(getStructure(CameraView::structureTypeName, name));
}

RaySet* getRaySet(std::string name) { return dynamic_cast<RaySet*>(getStructure(RaySet::structureTypeName, name)); }

void removeStructure(std::string type, std::string name, bool errorIfAbsent) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    if (errorIfAbsent) {
      error("No structures of type " + type + " registered");
    }
    return;
  }
  std::map<std::string, Structure*>& sMap = state::structures[type];

  // Check if structure exists
  if (sMap.find(name) == sMap.end()) {
    if (errorIfAbsent) {
      error("No structure of type " + type + " and name " + name + " registered");
    }
    return;
  }

  // Structure exists, remove it
  Structure* s = sMap[name];
  pick::clearPickIfStructureSelected(s);
  sMap.erase(s->name);
  delete s;
  updateStructureExtents();
  return;
}

void removeStructure(std::string name) {

  // Check if we can find exactly one structure matching the name
  Structure* targetStruct = nullptr;
  for (auto typeMap : state::structures) {
    for (auto entry : typeMap.second) {

      // Found a matching structure
      if (entry.first == name) {
        if (targetStruct == nullptr) {
          targetStruct = entry.second;
        } else {
          error("Cannot use automatic structure remove with empty name unless there is exactly one structure of that "
                "type registered. Found two structures of different types with that name: " +
                targetStruct->type + " and " + typeMap.first + ".");
          return;
        }
      }
    }
  }

  // Error if none found.
  if (targetStruct == nullptr) {
    error("No structure named: " + name + " to remove.");
    return;
  }

  removeStructure(targetStruct->type, targetStruct->name);
}

void removeAllStructures() {

  for (auto typeMap : state::structures) {

    // dodge iterator invalidation
    std::vector<std::string> names;
    for (auto entry : typeMap.second) {
      names.push_back(entry.first);
    }

    // remove all
    for (auto name : names) {
      removeStructure(typeMap.first, name);
    }
  }

  pick::resetPick();
}

void updateStructureExtents() {
  // Compute length scale and bbox as the max of all structures
  state::lengthScale = 0.0;
  Vector3 minBbox = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 maxBbox = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (auto cat : state::structures) {
    for (auto x : cat.second) {
      state::lengthScale = std::max(state::lengthScale, x.second->lengthScale());
      auto bbox = x.second->boundingBox();
      minBbox = geometrycentral::componentwiseMin(minBbox, std::get<0>(bbox));
      maxBbox = geometrycentral::componentwiseMax(maxBbox, std::get<1>(bbox));
    }
  }

  if (!minBbox.isFinite() || !maxBbox.isFinite()) {
    minBbox = -Vector3{1, 1, 1};
    maxBbox = Vector3{1, 1, 1};
  }
  std::get<0>(state::boundingBox) = minBbox;
  std::get<1>(state::boundingBox) = maxBbox;

  // If we got a bounding box but not a length scale we can use the size of the
  // box as a scale. If we got neither, we'll end up with a constant near 1 due
  // to the above correction
  if (state::lengthScale == 0) {
    state::lengthScale = norm(maxBbox - minBbox);
  }

  // Center is center of bounding box
  state::center = 0.5 * (minBbox + maxBbox);
}

void screenshot(std::string filename, bool transparentBG) {

  // Make sure we render first
  draw(false);

  // Get buffer size
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int w = viewport[2];
  int h = viewport[3];

  // Read from openGL
  size_t buffSize = w * h * 4;
  unsigned char* buff = new unsigned char[buffSize];
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buff);

  // Just flip
  if (transparentBG) {

    size_t flipBuffSize = w * h * 4;
    unsigned char* flipBuff = new unsigned char[flipBuffSize];
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        int flipInd = i + (h - j - 1) * w;
        flipBuff[4 * flipInd + 0] = buff[4 * ind + 0];
        flipBuff[4 * flipInd + 1] = buff[4 * ind + 1];
        flipBuff[4 * flipInd + 2] = buff[4 * ind + 2];
        flipBuff[4 * flipInd + 3] = buff[4 * ind + 3];
      }
    }

    // Save to file
    saveImage(filename, flipBuff, w, h, 4);

    delete[] flipBuff;
  }
  // Strip alpha channel and flip
  else {

    size_t noAlphaBuffSize = w * h * 3;
    unsigned char* noAlphaBuff = new unsigned char[noAlphaBuffSize];
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        int flipInd = i + (h - j - 1) * w;
        noAlphaBuff[3 * flipInd + 0] = buff[4 * ind + 0];
        noAlphaBuff[3 * flipInd + 1] = buff[4 * ind + 1];
        noAlphaBuff[3 * flipInd + 2] = buff[4 * ind + 2];
      }
    }

    // Save to file
    saveImage(filename, noAlphaBuff, w, h, 3);

    delete[] noAlphaBuff;
  }

  delete[] buff;
}

void screenshot(bool transparentBG) {

  char buff[50];
  snprintf(buff, 50, "screenshot_%06zu.png", state::screenshotInd);
  std::string defaultName(buff);

  screenshot(defaultName, transparentBG);

  state::screenshotInd++;
}

} // namespace polyscope

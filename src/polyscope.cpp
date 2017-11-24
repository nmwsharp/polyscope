#include "polyscope/polyscope.h"

#include <iostream>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "imgui.h"
#include "polyscope/imgui_render.h"

#include "polyscope/view.h"

namespace polyscope {

// === Declare storage global members

namespace state {

bool initialized = false;
std::unordered_set<std::string> allStructureNames;
std::map<std::string, PointCloud*> pointClouds;
double lengthScale = 1.0;

}  // namespace state

namespace options {

std::string programName = "Polyscope";
int verbosity = 2;
std::string printPrefix = "Polyscope: ";
bool exceptionOnError = true;

}  // namespace options

// Small callback function for GLFW errors
void error_print_callback(int error, const char* description) {
  std::cerr << "GLFW emitted error: " << description << std::endl;
}

// === Core global functions

void init() {
  if (state::initialized) {
    throw std::logic_error(options::printPrefix + "Initialize called twice");
  }

  // === Initialize glfw
  glfwSetErrorCallback(error_print_callback);
  if (!glfwInit()) {
    throw std::runtime_error(options::printPrefix +
                             "ERROR: Failed to initialize glfw");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  imguirender::mainWindow =
      glfwCreateWindow(1280, 720, options::programName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(imguirender::mainWindow);
  glfwSwapInterval(1);  // Enable vsync

  // === Initialize openGL
  // Load openGL functions (using GLAD)
#ifndef __APPLE__
  if (!gladLoadGL()) {
    throw std::runtime_error(options::printPrefix +
                             "ERROR: Failed to load openGL using GLAD");
  }
#endif
  if (options::verbosity > 0) {
    std::cout << options::printPrefix
              << "Loaded openGL version: " << glGetString(GL_VERSION)
              << std::endl;
  }

#ifdef __APPLE__
  // Hack to classify the process as interactive
  glfwPollEvents();
#endif

  // Set up ImGUI glfw bindings
  imguirender::ImGui_ImplGlfwGL3_Init(imguirender::mainWindow, true);

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig config;
  config.OversampleH = 5;
  config.OversampleV = 5;
  // io.Fonts->AddFontDefault();
  io.Fonts->AddFontFromFileTTF(
      "../deps/imgui/imgui/extra_fonts/Cousine-Regular.ttf", 15.0f, &config);

  // Initialize common shaders
  gl::GLProgram::initCommonShaders();

  state::initialized = true;
}

namespace {

void processMouseEvents() {

  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {

    // Handle drags
    if(io.MouseDown[0]) {

      Vector2 dragDelta{io.MouseDelta.x / view::windowWidth, -io.MouseDelta.y / view::windowHeight};
      view::processMouseDrag(dragDelta, !io.KeyShift);

    }

  }
}

void drawStructures() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  for (auto x : state::pointClouds) {
    x.second->draw();
  }
}

void buildPolyscopeGui() {

  // Create window
  static bool showPolyscopeWindow = true;
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::Begin("Polyscope", &showPolyscopeWindow, ImGuiWindowFlags_AlwaysAutoResize);


  ImGui::ColorEdit3("background color", (float*)&view::bgColor,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::Text("%.1f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);


  ImGui::End();
}

void buildStructureGui() {

  // Create window
  static bool showStructureWindow = true;
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::Begin("Structures", &showStructureWindow);
  // ImGui::Begin("Structures", &showStructureWindow, ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::SetNextTreeNodeOpen(0 > 0, ImGuiCond_FirstUseEver);
  if (ImGui::CollapsingHeader(("Surface Meshes (" + std::to_string(0) + ")").c_str())) {
    ImGui::Text("hi mom");
  }

  // Draw point clouds 
  ImGui::SetNextTreeNodeOpen(state::pointClouds.size() > 0, ImGuiCond_FirstUseEver);
  if (ImGui::CollapsingHeader(("Point Clouds (" + std::to_string(state::pointClouds.size()) + ")").c_str())) {
    for (auto x : state::pointClouds) {
      x.second->drawUI();
    }
  }


  ImGui::End();
}

}  // anonymous namespace

void show() {
  bool show_test_window = true;
  bool show_another_window = false;

  // Main loop
  while (!glfwWindowShouldClose(imguirender::mainWindow)) {
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.

    // Update the width and heigh
    glfwMakeContextCurrent(imguirender::mainWindow);
    glfwGetWindowSize(imguirender::mainWindow, &view::windowWidth,
                      &view::windowHeight);
    glfwGetFramebufferSize(imguirender::mainWindow, &view::bufferWidth,
                           &view::bufferHeight);

    glfwPollEvents();
    imguirender::ImGui_ImplGlfwGL3_NewFrame();

    processMouseEvents();

    // Build the GUI components
    buildPolyscopeGui();
    buildStructureGui();

    // Process UI events


    // TODO handle picking if needed

    // === Rendering

    // Clear out the gui
    glViewport(0, 0, view::bufferWidth, view::bufferHeight);
    glClearColor(view::bgColor[0], view::bgColor[1], view::bgColor[2],
                 view::bgColor[3]);
    glClearDepth(1.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Draw structures in the scene
    drawStructures();

    // Draw the GUI
    ImGui::Render();

    glfwSwapBuffers(imguirender::mainWindow);
  }
}

void registerPointCloud(std::string name, const std::vector<Vector3>& points) {
  // Make sure the name has not already been used
  if (state::allStructureNames.find(name) != state::allStructureNames.end()) {
    error("Structure name " + name + " is already in used");
  }

  // Add the new point cloud
  state::pointClouds[name] = new PointCloud(name, points);
  state::allStructureNames.insert(name);

  computeLengthScale();
}

void removeStructure(std::string name) {
  if (state::pointClouds.find(name) != state::pointClouds.end()) {
    delete state::pointClouds[name];
    state::pointClouds.erase(name);
    state::allStructureNames.erase(name);
    computeLengthScale();
    return;
  }

  error("No structure named: " + name + " to remove.");
}

void removeAllStructures() {
  for (auto x : state::pointClouds) delete x.second;
  state::pointClouds.clear();
  state::allStructureNames.clear();
  computeLengthScale();
}

void computeLengthScale() {
  // Default to 1.0;
  if (state::allStructureNames.size() == 0) {
    state::lengthScale = 1.0;
    return;
  }

  // Compute as the max of all structures
  state::lengthScale = 0.0;
  for (auto x : state::pointClouds) {
    state::lengthScale = std::max(state::lengthScale, x.second->lengthScale());
  } 

  if(state::lengthScale == 0) state::lengthScale = 1.0;

  // state::lengthScale = .3; 
  // state::lengthScale = 1.099; 
  std::cout << "scale = " << state::lengthScale << std::endl;
}

void error(std::string message) {
  if (options::exceptionOnError) {
    throw std::logic_error(options::printPrefix + message);
  } else {
    std::cout << options::printPrefix << message << std::endl;
  }
}

}  // namespace polyscope
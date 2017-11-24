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
std::unordered_map<std::string, PointCloud*> pointClouds;
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
  imgui::mainWindow =
      glfwCreateWindow(1280, 720, options::programName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(imgui::mainWindow);
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
  imgui::ImGui_ImplGlfwGL3_Init(imgui::mainWindow, true);

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

void drawStructures() {
    
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); 

  for (auto x : state::pointClouds) {
    x.second->draw();
  }

}

}  // anonymous namespace

void show() {
  bool show_test_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Main loop
  while (!glfwWindowShouldClose(imgui::mainWindow)) {
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.

    // Update the width and heigh
    glfwMakeContextCurrent(imgui::mainWindow);
    glfwGetWindowSize(imgui::mainWindow, &view::windowWidth, &view::windowHeight);
    glfwGetFramebufferSize(imgui::mainWindow, &view::bufferWidth, &view::bufferHeight);

    glfwPollEvents();
    imgui::ImGui_ImplGlfwGL3_NewFrame();

    // 1. Show a simple window.
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in
    // a window automatically called "Debug".
    {
      static float f = 0.0f;
      ImGui::Text("Hello, world!");
      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
      ImGui::ColorEdit3("clear color", (float*)&clear_color);
      if (ImGui::Button("Test Window")) show_test_window ^= 1;
      if (ImGui::Button("Another Window")) show_another_window ^= 1;
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window. In most cases you will use an explicit
    // Begin/End pair to name the window.
    if (show_another_window) {
      ImGui::Begin("Another Window", &show_another_window);
      ImGui::Text("Hello from another window!");
      ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in
    // ImGui::ShowTestWindow().
    if (show_test_window) {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
      ImGui::ShowTestWindow(&show_test_window);
    }

    // TODO handle picking if needed

    // === Rendering

    // Clear out the gui
    glViewport(0, 0, view::bufferWidth, view::bufferHeight);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClearDepth( 1. );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Draw structures in the scene
    drawStructures();

    // Draw the GUI
    ImGui::Render();


    glfwSwapBuffers(imgui::mainWindow);
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
}

void error(std::string message) {
  if (options::exceptionOnError) {
    throw std::logic_error(options::printPrefix + message);
  } else {
    std::cout << options::printPrefix << message << std::endl;
  }
}

}  // namespace polyscope
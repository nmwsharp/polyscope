// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED

#include "polyscope/render/opengl/gl_engine_glfw.h"

#include "backends/imgui_impl_opengl3.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include <algorithm>
#include <set>

namespace polyscope {
namespace render {
namespace backend_openGL3 {

void initializeRenderEngine_glfw() {

  GLEngineGLFW* glEngineGLFW = new GLEngineGLFW(); // create the new global engine object

  engine = glEngineGLFW; // we keep a few copies of this pointer with various types

  // initialize
  glEngineGLFW->initialize();
  engine->allocateGlobalBuffersAndPrograms();
}

GLEngineGLFW::GLEngineGLFW() {}
GLEngineGLFW::~GLEngineGLFW() {}

void GLEngineGLFW::initialize() {

  // Small callback function for GLFW errors
  auto error_print_callback = [](int error, const char* description) {
    if (polyscope::options::verbosity > 0) {
      std::cout << "GLFW emitted error: " << description << std::endl;
    }
  };

  // === Initialize glfw
  glfwSetErrorCallback(error_print_callback);
  if (!glfwInit()) {
    exception("ERROR: Failed to initialize glfw");
  }

  // OpenGL version things
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Create the window with context
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
  mainWindow = glfwCreateWindow(view::windowWidth, view::windowHeight, options::programName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(mainWindow);
  glfwSetWindowPos(mainWindow, view::initWindowPosX, view::initWindowPosY);

  // Set initial window size
  int newBufferWidth, newBufferHeight, newWindowWidth, newWindowHeight;
  glfwGetFramebufferSize(mainWindow, &newBufferWidth, &newBufferHeight);
  glfwGetWindowSize(mainWindow, &newWindowWidth, &newWindowHeight);
  view::bufferWidth = newBufferWidth;
  view::bufferHeight = newBufferHeight;
  view::windowWidth = newWindowWidth;
  view::windowHeight = newWindowHeight;

  setWindowResizable(view::windowResizable);

// === Initialize openGL
// Load openGL functions (using GLAD)
#ifndef __APPLE__
  if (!gladLoadGL()) {
    exception("ERROR: Failed to load openGL using GLAD");
  }
#endif
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "Backend: openGL3_glfw -- "
              << "Loaded openGL version: " << glGetString(GL_VERSION) << std::endl;
  }

#ifdef __APPLE__
  // Hack to classify the process as interactive
  glfwPollEvents();
#endif

  { // Manually create the screen frame buffer
    GLFrameBuffer* glScreenBuffer = new GLFrameBuffer(view::bufferWidth, view::bufferHeight, true);
    displayBuffer.reset(glScreenBuffer);
    glScreenBuffer->bind();
    glClearColor(1., 1., 1., 0.);
    // glClearColor(0., 0., 0., 0.);
    // glClearDepth(1.);
  }

  populateDefaultShadersAndRules();
}


void GLEngineGLFW::initializeImGui() {
  bindDisplay();

  ImGui::CreateContext(); // must call once at start

  // Set up ImGUI glfw bindings
  ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
  const char* glsl_version = "#version 150";
  ImGui_ImplOpenGL3_Init(glsl_version);

  configureImGui();
}


void GLEngineGLFW::shutdown() {
  checkError();
  shutdownImGui();
  glfwDestroyWindow(mainWindow);
  glfwTerminate();
}


void GLEngineGLFW::shutdownImGui() {
  // ImGui shutdown things
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void GLEngineGLFW::ImGuiNewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void GLEngineGLFW::ImGuiRender() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  clearResourcesPreservedForImguiFrame();
}


void GLEngineGLFW::swapDisplayBuffers() {
  bindDisplay();
  glfwSwapBuffers(mainWindow);
}


void GLEngineGLFW::makeContextCurrent() {
  glfwMakeContextCurrent(mainWindow);
  glfwSwapInterval(options::enableVSync ? 1 : 0);
}

void GLEngineGLFW::focusWindow() { glfwFocusWindow(mainWindow); }

void GLEngineGLFW::showWindow() { glfwShowWindow(mainWindow); }

void GLEngineGLFW::hideWindow() {
  glfwHideWindow(mainWindow);
  glfwPollEvents(); // this shouldn't be necessary, but seems to be needed at least on macOS. Perhaps realted to a
                    // glfw bug? e.g. https://github.com/glfw/glfw/issues/1300 and related bugs
}

void GLEngineGLFW::updateWindowSize(bool force) {
  int newBufferWidth, newBufferHeight, newWindowWidth, newWindowHeight;
  glfwGetFramebufferSize(mainWindow, &newBufferWidth, &newBufferHeight);
  glfwGetWindowSize(mainWindow, &newWindowWidth, &newWindowHeight);
  if (force || newBufferWidth != view::bufferWidth || newBufferHeight != view::bufferHeight ||
      newWindowHeight != view::windowHeight || newWindowWidth != view::windowWidth) {
    // Basically a resize callback
    requestRedraw();

    // prevent any division by zero for e.g. aspect ratio calcs
    if (newBufferHeight == 0) newBufferHeight = 1;
    if (newWindowHeight == 0) newWindowHeight = 1;

    view::bufferWidth = newBufferWidth;
    view::bufferHeight = newBufferHeight;
    view::windowWidth = newWindowWidth;
    view::windowHeight = newWindowHeight;

    render::engine->resizeScreenBuffers();
    render::engine->setScreenBufferViewports();
  }
}


void GLEngineGLFW::applyWindowSize() {
  glfwSetWindowSize(mainWindow, view::windowWidth, view::windowHeight);

  // on some platform size changes are asynchonous, need to ensure it completes
  // we don't want to just retry until the resize has happened, because it could be impossible
  // TODO it seems like on X11 sometimes even this isn't enough?
  // glfwWaitEvents();
  // NSHARP: disabling this ^^^, on macOS it is causing the window to block until it gets focus,
  // and some googling makes me thing the underlying buffers should be resized immediately, which is
  // all we really care about

  updateWindowSize(true);
}


void GLEngineGLFW::setWindowResizable(bool newVal) {
  glfwSetWindowAttrib(mainWindow, GLFW_RESIZABLE, newVal ? GLFW_TRUE : GLFW_FALSE);
}

bool GLEngineGLFW::getWindowResizable() { return glfwGetWindowAttrib(mainWindow, GLFW_RESIZABLE); }

std::tuple<int, int> GLEngineGLFW::getWindowPos() {
  int x, y;
  glfwGetWindowPos(mainWindow, &x, &y);
  return std::tuple<int, int>{x, y};
}

bool GLEngineGLFW::windowRequestsClose() {
  bool shouldClose = glfwWindowShouldClose(mainWindow);
  if (shouldClose) {
    glfwSetWindowShouldClose(mainWindow, false); // un-set the state bit so we can close again
    return true;
  }
  return false;
}

void GLEngineGLFW::pollEvents() { glfwPollEvents(); }

bool GLEngineGLFW::isKeyPressed(char c) {
  if (c >= '0' && c <= '9') return ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0')));
  if (c >= 'a' && c <= 'z') return ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_A + (c - 'a')));
  if (c >= 'A' && c <= 'Z') return ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A')));
  exception("keyPressed only supports 0-9, a-z, A-Z");
  return false;
}

std::string GLEngineGLFW::getClipboardText() {
  std::string clipboardData = ImGui::GetClipboardText();
  return clipboardData;
}

void GLEngineGLFW::setClipboardText(std::string text) { ImGui::SetClipboardText(text.c_str()); }

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

#else // POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED

#include "polyscope/messages.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

void initializeRenderEngine_glfw() { exception("Polyscope was not compiled with support for backend: openGL3_glfw"); }

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

#endif

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED

#include "polyscope/render/opengl/gl_engine_glfw.h"

#include "backends/imgui_impl_opengl3.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include <algorithm>
#include <set>
#include <sstream>

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
    info(0, "GLFW emitted error: " + std::string(description));
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

  // This tells GLFW to scale window size/positioning/content based on the system-reported DPI scaling factor
  // However, it can lead to some confusing behaviors, for instance, on linux with scaling=200%, if the user
  // sets view::windowWidth = 1280, they might get back a window with windowWidth == bufferWidth == 2560,
  // which is quite confusing.
  // For this reason we _do not_ set this hint. If desired, the user can specify a windowWidth = 1280*uiScale,
  // or let the window size by loaded from .polyscope.ini after setting manually once.
  // glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

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

  {
    std::stringstream ss;
    ss << "Backend: openGL3_glfw -- "
       << "Loaded openGL version: " << glGetString(GL_VERSION);
    info(0, ss.str());
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

  // Set the UI scale to account for system-requested DPI scaling
  // Currently we do *not* watch for changes of this value e.g. if a window moves between
  // monitors with different DPI behaviors. We could, but it would require some logic to
  // avoid overwriting values that a user might have set.
  if (options::uiScale < 0) { // only set from system if the value is -1, meaning not set yet
    setUIScaleFromSystemDPI();
  }

  populateDefaultShadersAndRules();
}

void GLEngineGLFW::setUIScaleFromSystemDPI() {

  // logic adapted from a helpful imgui issue here: https://github.com/ocornut/imgui/issues/6967#issuecomment-2833882081

  ImVec2 windowSize{static_cast<float>(view::windowWidth), static_cast<float>(view::windowHeight)};
  ImVec2 bufferSize{static_cast<float>(view::bufferWidth), static_cast<float>(view::bufferHeight)};
  ImVec2 imguiCoordScale = {bufferSize.x / windowSize.x, bufferSize.y / windowSize.y};

  ImVec2 contentScale;
  glfwGetWindowContentScale(mainWindow, &contentScale.x, &contentScale.y);

  float sx = contentScale.x / imguiCoordScale.x;
  float sy = contentScale.y / imguiCoordScale.y;
  options::uiScale = std::max(sx, sy);
  // clamp to values within [0.5x,4x] scaling
  options::uiScale = std::fmin(std::fmax(options::uiScale, 0.5f), 4.0f);

  info(100, "window size: " + std::to_string(view::windowWidth) + "," + std::to_string(view::windowHeight));
  info(100, "buffer size: " + std::to_string(view::bufferWidth) + "," + std::to_string(view::bufferHeight));
  info(100, "imguiCoordScale: " + std::to_string(imguiCoordScale.x) + "," + std::to_string(imguiCoordScale.y));
  info(100, "contentScale: " + std::to_string(contentScale.x) + "," + std::to_string(contentScale.y));
  info(100, "computed uiScale: " + std::to_string(options::uiScale));
}

void GLEngineGLFW::initializeImGui() {
  bindDisplay();

  ImGui::CreateContext(); // must call once at start
  ImPlot::CreateContext(); 

  // Set up ImGUI glfw bindings
  ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
  const char* glsl_version = "#version 150";
  ImGui_ImplOpenGL3_Init(glsl_version);

  configureImGui();
}

void GLEngineGLFW::configureImGui() {

  if (options::uiScale < 0) {
    exception("uiScale is < 0. Perhaps it wasn't initialized?");
  }

  if (options::prepareImGuiFontsCallback) {

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // these are necessary if different fonts are loaded in the callback
    // (don't totally understand why, allegedly it may change in the future)
    ImGui_ImplOpenGL3_DestroyFontsTexture();

    ImFontAtlas* _unused;
    std::tie(_unused, regularFont, monoFont) = options::prepareImGuiFontsCallback();

    ImGui_ImplOpenGL3_CreateFontsTexture();
  }


  if (options::configureImGuiStyleCallback) {
    options::configureImGuiStyleCallback();
  }
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
  ImPlot::DestroyContext();
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


// For some reason the ImGui::SetClipboardText() didn't work here, on linux
// it would not actually write to system clipboard. Calling glfw directly seems to work.

std::string GLEngineGLFW::getClipboardText() {
  std::string clipboardData = glfwGetClipboardString(nullptr);
  return clipboardData;
}

void GLEngineGLFW::setClipboardText(std::string text) { glfwSetClipboardString(nullptr, text.c_str()); }

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

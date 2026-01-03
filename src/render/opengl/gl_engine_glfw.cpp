// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED

#include "polyscope/render/opengl/gl_engine_glfw.h"

#include "backends/imgui_impl_glfw_polyscope.h"
#include "backends/imgui_impl_opengl3.h"
#include "polyscope/imgui_config.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include "ImGuizmo.h"

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

  // Drag & drop support
  glfwSetDropCallback(mainWindow, [](GLFWwindow* window, int path_count, const char* paths[]) {
    if (!state::filesDroppedCallback) return;

    std::vector<std::string> pathsVec(path_count);
    for (int i = 0; i < path_count; i++) {
      pathsVec[i] = paths[i];
    }

    state::filesDroppedCallback(pathsVec);
  });


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

  // the ImGui backend has built-in logic for detecting scale via GLFW, including (allegedly) handling
  // macOS & Windows idiosyncrasies correctly
  options::uiScale = ImGui_ImplGlfw_GetContentScaleForWindow(mainWindow);

  // clamp to values within [0.5x,4x] scaling
  options::uiScale = std::fmin(std::fmax(options::uiScale, 0.5f), 4.0f);

  info(100, "computed uiScale: " + std::to_string(options::uiScale));
}

void GLEngineGLFW::createNewImGuiContext() {
  bindDisplay();

  ImGuiContext* newContext = ImGui::CreateContext(imguiInitialized ? sharedFontAtlas : nullptr);
  ImGui::SetCurrentContext(newContext);

  // Set up ImGUI glfw bindings
  ImGui_ImplGlfw_InitForOpenGL(mainWindow, !imguiInitialized);
  const char* glsl_version = "#version 150";
  ImGui_ImplOpenGL3_Init(glsl_version);

  if (!imguiInitialized) {
    // the font atlas from the base context is used by all others
    sharedFontAtlas = ImGui::GetIO().Fonts;

    if (options::prepareImGuiFontsCallback) {
      std::tie(regularFont, monoFont) = options::prepareImGuiFontsCallback(sharedFontAtlas);
    }
  }

  ImPlotContext* newPlotContext = ImPlot::CreateContext();
  ImPlot::SetCurrentContext(newPlotContext);

  configureImGui();

  if (!imguiInitialized) {
    // Immediately open and close a frame, this forces imgui to populate its fonts and other data
    //
    // Otherwise, we get errors on show(), because we create a new context sharing the same atlas,
    // when that context tries to render it errors out because its atlas is not populated. (Observed
    // in ImGui 1.92.5)
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = 1000;
    io.DisplaySize.y = 1000;
    ImGui::NewFrame();
    ImGui::EndFrame();

    imguiInitialized = true;
  }
}

void GLEngineGLFW::updateImGuiContext(ImGuiContext* newContext) {
  ImGui_ImplGlfw_ContextMap_UpdateIfPresent(mainWindow, newContext);
}

void GLEngineGLFW::configureImGui() {

  if (options::uiScale < 0) {
    exception("uiScale is < 0. Perhaps it wasn't initialized?");
  }

  ImGuiIO& io = ImGui::GetIO();

  // TODO these seem to be documented but don't actually exist in ImGui 1.92.5
  // It would be good to use ImGui's built-in scaling behavior, but for now Polyscope 
  // has lots of built-in window size logic that requires the window size to match the 
  // font size to look reasonable, so we can't just let the font size get changed 
  // automatically.
  // io.ConfigDpiScaleFonts = false; 
  // io.ConfigDpiScaleViewports = false;

  // if polyscope's prefs file is disabled, disable imgui's ini file too
  if (!options::usePrefsFile) {
    io.IniFilename = nullptr;
  }

  if (options::configureImGuiStyleCallback) {
    options::configureImGuiStyleCallback();
  }
}


void GLEngineGLFW::shutdown() {
  freeAllOwnedResources();
  checkError();
  shutdownImGui();
  checkError();
  glfwDestroyWindow(mainWindow);
  // no checkError() after this, openGL has been unloaded
  glfwTerminate();
}


void GLEngineGLFW::shutdownImGui() {
  ImGui_ImplGlfw_RestoreCallbacks(mainWindow);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  imguiInitialized = false;
  sharedFontAtlas = nullptr;
  regularFont = nullptr;
  monoFont = nullptr;
}

void GLEngineGLFW::ImGuiNewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();
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

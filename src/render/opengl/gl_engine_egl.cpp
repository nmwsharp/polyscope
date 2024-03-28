// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL3_EGL_ENABLED

#include "polyscope/polyscope.h"

#include "polyscope/render/opengl/gl_engine_egl.h"

#include "backends/imgui_impl_opengl3.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include <algorithm>
#include <set>

namespace polyscope {
namespace render {
namespace backend_openGL3 {

GLEngineEGL* glEngineEGL = nullptr; // alias for global engine pointer
extern GLEngine* glEngine;          // defined in gl_engine.h

namespace { // anonymous helpers

void checkEGLError(bool fatal = true) {

  if (!options::enableRenderErrorChecks) {
    return;
  }

  // Map the GL error enums to strings
  EGLint err = eglGetError();

  if (err == EGL_SUCCESS) return;

  std::string errText;
  switch (err) {

  case EGL_SUCCESS:
    errText = "The last function succeeded without error.";
    break;

  case EGL_NOT_INITIALIZED:
    errText = "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
    break;

  case EGL_BAD_ACCESS:
    errText = "EGL cannot access a requested resource (for example a context is bound in another thread).";
    break;

  case EGL_BAD_ALLOC:
    errText = "EGL failed to allocate resources for the requested operation.";
    break;

  case EGL_BAD_ATTRIBUTE:
    errText = "An unrecognized attribute or attribute value was passed in the attribute list.";
    break;

  case EGL_BAD_CONTEXT:
    errText = "An EGLContext argument does not name a valid EGL rendering context.";
    break;

  case EGL_BAD_CONFIG:
    errText = "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
    break;

  case EGL_BAD_CURRENT_SURFACE:
    errText = "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.";
    break;

  case EGL_BAD_DISPLAY:
    errText = "An EGLDisplay argument does not name a valid EGL display connection.";
    break;

  case EGL_BAD_SURFACE:
    errText = "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL "
              "rendering.";
    break;

  case EGL_BAD_MATCH:
    errText =
        "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).";
    break;

  case EGL_BAD_PARAMETER:
    errText = "One or more argument values are invalid.";
    break;

  case EGL_BAD_NATIVE_PIXMAP:
    errText = "A NativePixmapType argument does not refer to a valid native pixmap.";
    break;

  case EGL_BAD_NATIVE_WINDOW:
    errText = "A NativeWindowType argument does not refer to a valid native window.";
    break;

  case EGL_CONTEXT_LOST:
    errText = "A power management event has occurred. The application must destroy all contexts and reinitialise "
              "OpenGL ES state and objects to continue rendering.";
    break;

  default:
    errText = "Unknown error " + std::to_string(static_cast<unsigned int>(err));
    break;
  }

  if (polyscope::options::verbosity > 0) {
    std::cout << polyscope::options::printPrefix << "EGL Error!  Type: " << errText << std::endl;
  }
  if (fatal) {
    exception("EGL error occurred. Text: " + errText);
  }
}
} // namespace

void initializeRenderEngine_egl() {

  glEngineEGL = new GLEngineEGL(); // create the new global engine object

  engine = glEngineEGL; // we keep a few copies of this pointer with various types
  glEngine = glEngineEGL;

  // initialize
  glEngineEGL->initialize();
  engine->allocateGlobalBuffersAndPrograms();
  glEngineEGL->applyWindowSize();
}

GLEngineEGL::GLEngineEGL() {}
GLEngineEGL::~GLEngineEGL() {
  // eglTerminate(eglDisplay) // TODO handle termination
}

void GLEngineEGL::initialize() {

  // === Initialize EGL

  // Get the default display
  eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglDisplay == EGL_NO_DISPLAY) {
    exception("ERROR: Failed to initialize EGL, could not get default display");
  }

  // Configure
  EGLint majorVer, minorVer;
  bool success = eglInitialize(eglDisplay, &majorVer, &minorVer);
  if (!success) {
    checkEGLError(false);
    exception("ERROR: Failed to initialize EGL");
  }
  checkEGLError();


  // this has something to do with the EGL configuration, I don't understand exactly what
  // clang-format off
  const EGLint configAttribs[] = {
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, 
      EGL_BLUE_SIZE, 8, 
      EGL_GREEN_SIZE, 8, 
      EGL_RED_SIZE, 8, 
      EGL_DEPTH_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,   // this is important, it gets us openGL rather than openGL ES
  EGL_NONE};
  // clang-format on


  EGLint numConfigs;
  EGLConfig eglCfg;
  eglChooseConfig(eglDisplay, configAttribs, &eglCfg, 1, &numConfigs);
  checkEGLError();

  eglBindAPI(EGL_OPENGL_API);
  checkEGLError();

  // requested context configuration (openGL 3.3 core profile)
  // clang-format off
  EGLint contextAttribs[] = {
      EGL_CONTEXT_MAJOR_VERSION, 3,
      EGL_CONTEXT_MINOR_VERSION, 3,
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
  EGL_NONE };
  // clang-format on

  eglContext = eglCreateContext(eglDisplay, eglCfg, EGL_NO_CONTEXT, contextAttribs);
  checkEGLError();

  makeContextCurrent();

  view::bufferWidth = view::windowWidth;
  view::bufferHeight = view::windowHeight;


// === Initialize openGL
// Load openGL functions (using GLAD)
//
// NOTE: right now this is using the same glad loader as the standard gl_engine_glfw.cpp uses, which was generated with
// the "OpenGL" specification setting rather than the "EGL" option. One would think we should use the "EGL"
// specification, however this one seems to work and that one does not.
//
#ifndef __APPLE__
  if (!gladLoadGL()) {
    exception(options::printPrefix + "ERROR: Failed to load openGL using GLAD");
  }
#endif
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "Backend: openGL3_egl -- "
              << "Loaded openGL version: " << glGetString(GL_VERSION) << " -- "
              << "EGL version: " << majorVer << "." << minorVer << std::endl;
  }

  { // Manually create the screen frame buffer
    // NOTE: important difference here, we manually create both the framebuffer and and its render buffer, since
    // headless EGL means we are not getting them from a window
    displayBuffer = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    displayBuffer->addColorBuffer(
        generateRenderBuffer(RenderBufferType::Float4, view::bufferWidth, view::bufferHeight));
    displayBuffer->addDepthBuffer(generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight));
    displayBuffer->setDrawBuffers();
    checkError();

    displayBuffer->bind();
    glClearColor(1., 1., 1., 0.);

    checkError();
  }

  populateDefaultShadersAndRules();
  checkError();
}


void GLEngineEGL::initializeImGui() {

  // headless mode uses the "null" imgui backed, which essentially does nothing and just passes-through inputs to all
  // functions

  ImGui::CreateContext();
  configureImGui();
}

void GLEngineEGL::shutdownImGui() { ImGui::DestroyContext(); }

void GLEngineEGL::ImGuiNewFrame() {

  // ImGUI has an error check which fires unless we do this
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize.x = view::bufferWidth;
  io.DisplaySize.y = view::bufferHeight;

  ImGui::NewFrame();
}

void GLEngineEGL::ImGuiRender() { ImGui::Render(); }


void GLEngineEGL::swapDisplayBuffers() {
  // not defined in headless mode
}

void GLEngineEGL::checkError(bool fatal) {
  checkEGLError(fatal);
  GLEngine::checkError(fatal); // call the parent version (checks openGL error)
}

void GLEngineEGL::makeContextCurrent() {
  eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);
  checkEGLError();
}

void GLEngineEGL::focusWindow() {
  // not defined in headless mode
}

void GLEngineEGL::showWindow() {
  // not defined in headless mode
}

void GLEngineEGL::hideWindow() {
  // not defined in headless mode
}

void GLEngineEGL::updateWindowSize(bool force) {
  // does nothing
  // (this function is for updating polyscope's internal window & buffer info in response to the user resizing the
  // window at the OS level, but in headless mode there is no interactable window for the user to change size of)
}


void GLEngineEGL::applyWindowSize() {
  view::bufferWidth = view::windowWidth;
  view::bufferHeight = view::windowHeight;

  render::engine->resizeScreenBuffers();
  render::engine->setScreenBufferViewports();
}


void GLEngineEGL::setWindowResizable(bool newVal) {
  // not defined in headless mode
}

bool GLEngineEGL::getWindowResizable() { return false; }

std::tuple<int, int> GLEngineEGL::getWindowPos() {
  // not defined in headless mode
  return std::tuple<int, int>{-1, -1};
}

bool GLEngineEGL::windowRequestsClose() {
  // not defined in headless mode
  return false;
}

void GLEngineEGL::pollEvents() {
  // does nothing
}

bool GLEngineEGL::isKeyPressed(char c) {
  // not defined in headless mode
  return false;
}

int GLEngineEGL::getKeyCode(char c) {
  // not defined in headless mode
  return -1;
}

std::string GLEngineEGL::getClipboardText() {
  // not defined in headless mode
  return "";
}

void GLEngineEGL::setClipboardText(std::string text) {
  // not defined in headless mode
}

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

#else // POLYSCOPE_BACKEND_OPENGL3_EGL_ENABLED

#include "polyscope/messages.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

void initializeRenderEngine_egl() { exception("Polyscope was not compiled with support for backend: openGL3_egl"); }


} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

#endif

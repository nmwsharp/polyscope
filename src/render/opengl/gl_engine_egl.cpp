// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL3_EGL_ENABLED

#include "polyscope/polyscope.h"

#include "polyscope/render/opengl/gl_engine_egl.h"

#include "backends/imgui_impl_opengl3.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include <algorithm>
#include <cctype>
#include <dlfcn.h>
#include <set>
#include <string>

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace polyscope {
namespace render {
namespace backend_openGL3 {

void initializeRenderEngine_egl() {

  GLEngineEGL* glEngineEGL = new GLEngineEGL(); // create the new global engine object
  engine = glEngineEGL;

  // initialize
  glEngineEGL->initialize();
  engine->allocateGlobalBuffersAndPrograms();
  glEngineEGL->applyWindowSize();
}

GLEngineEGL::GLEngineEGL() {}
GLEngineEGL::~GLEngineEGL() {}

void GLEngineEGL::checkEGLError(bool fatal) {

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

void GLEngineEGL::initialize() {


  // === Initialize EGL

  // Runtime-load shared library functions for EGL
  // (see note inside for details)
  resolveEGL();

  // Query the available EGL devices
  const int N_MAX_DEVICE = 256;
  EGLDeviceEXT rawDevices[N_MAX_DEVICE];
  EGLint nDevices;
  if (!eglQueryDevicesEXT(N_MAX_DEVICE, rawDevices, &nDevices)) {
    error("EGL: Failed to query devices.");
  }
  if (nDevices == 0) {
    error("EGL: No devices found.");
  }
  info("EGL: Found " + std::to_string(nDevices) + " EGL devices.");

  // Build an ordered list of which devices to try initializing with
  std::vector<int32_t> deviceIndsToTry;
  if (options::eglDeviceIndex == -1) {
    info("EGL: No device index specified, attempting to intialize with each device in heuristic-guess order until "
         "success.");

    deviceIndsToTry.resize(nDevices);
    std::iota(deviceIndsToTry.begin(), deviceIndsToTry.end(), 0);
    sortAvailableDevicesByPreference(deviceIndsToTry, rawDevices);

  } else {
    info("EGL: Device index " + std::to_string(options::eglDeviceIndex) + " manually selected, using that device.");

    if (options::eglDeviceIndex >= nDevices) {
      error("EGL: Device index " + std::to_string(options::eglDeviceIndex) + " manually selected, but only " +
            std::to_string(nDevices) + " devices available.");
    }

    deviceIndsToTry.push_back(options::eglDeviceIndex);
  }

  bool successfulInit = false;
  EGLint majorVer, minorVer;
  for (int32_t iDevice : deviceIndsToTry) {

    info("EGL: Attempting initialization with device " + std::to_string(iDevice));
    EGLDeviceEXT device = rawDevices[iDevice];

    // Get an EGLDisplay for the device
    // (use the -platform / EXT version because it is the only one that seems to work in headless environments)
    eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, device, NULL);
    if (eglDisplay == EGL_NO_DISPLAY) {
      continue;
    }

    // Configure
    successfulInit = eglInitialize(eglDisplay, &majorVer, &minorVer);
    if (successfulInit) {
      break;
    }
  }

  if (!successfulInit) {
    exception("ERROR: Failed to initialize EGL");
  }
  checkEGLError();
  info("EGL: Initialization successful");

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

void GLEngineEGL::resolveEGL() {

  // This function does a bunch of gymnastics to avoid taking on libEGL.so as a dependency. This is a
  // machine/driver-specific library, so we would have to dynamically load it on the end user's machine. However,
  // simply specifying it as a shared library at build time would make it required for Polyscope, even for users
  // who would not use EGL, and we don't want that. Instead, we manually dynamically load the functions below.
  //
  // Note that this is on top of the dynamic loading that always happens when you load exention functions from libEGL.
  // We are furthermore loading `libEGL.so` dynamically in the middle of runtime, rather than at load time as via ldd
  // etc. At the end of this function we also load EGL extensions in the usual way.

  info(5, "Attempting to dlopen libEGL.so");
  void* handle = dlopen("libEGL.so", RTLD_LAZY);
  if (handle) info(5, "  ...loaded libEGL.so");
  if (!handle) {
    handle = dlopen("libEGL.so.1", RTLD_LAZY); // try it while we're at it, happens on OSMesa without dev headers
  }
  if (handle) info(5, "  ...loaded libEGL.so.1");
  if (!handle) {
    error(
        "EGL: Could not open libEGL.so. Ensure graphics drivers are installed, or fall back on (much slower!) software "
        "rendering by installing OSMesa from your package manager.");
  }

  // Get EGL functions
  info(5, "Attempting to dlsym resolve EGL functions");

  eglGetError = (eglGetErrorT)dlsym(handle, "eglGetError");
  eglGetPlatformDisplay = (eglGetPlatformDisplayT)dlsym(handle, "eglGetPlatformDisplay");
  eglChooseConfig = (eglChooseConfigT)dlsym(handle, "eglChooseConfig");
  eglInitialize = (eglInitializeT)dlsym(handle, "eglInitialize");
  eglChooseConfig = (eglChooseConfigT)dlsym(handle, "eglChooseConfig");
  eglBindAPI = (eglBindAPIT)dlsym(handle, "eglBindAPI");
  eglCreateContext = (eglCreateContextT)dlsym(handle, "eglCreateContext");
  eglMakeCurrent = (eglMakeCurrentT)dlsym(handle, "eglMakeCurrent");
  eglDestroyContext = (eglDestroyContextT)dlsym(handle, "eglDestroyContext");
  eglTerminate = (eglTerminateT)dlsym(handle, "eglTerminate");
  eglGetProcAddress = (eglGetProcAddressT)dlsym(handle, "eglGetProcAddress");
  eglQueryString = (eglQueryStringT)dlsym(handle, "eglQueryString");

  if (!eglGetError || !eglGetPlatformDisplay || !eglChooseConfig || !eglInitialize || !eglChooseConfig || !eglBindAPI ||
      !eglCreateContext || !eglMakeCurrent || !eglDestroyContext || !eglTerminate || !eglGetProcAddress ||
      !eglQueryString) {
    dlclose(handle);
    const char* errTextPtr = dlerror();
    std::string errText = "";
    if (errTextPtr) {
      errText = errTextPtr;
    }
    error("EGL: Error loading symbol " + errText);
  }
  if (options::verbosity > 5) {
    std::cout << polyscope::options::printPrefix << "  ...resolved EGL functions" << std::endl;
  }
  // ... at this point, we have essentially loaded libEGL.so

  // === Resolve EGL extension functions

  // Helper function to get an EGL extension function and error-check that
  // we got it successfully
  auto getEGLExtensionProcAndCheck = [&](std::string name) {
    void* procAddr = (void*)(eglGetProcAddress(name.c_str()));
    if (!procAddr) {
      error("EGL failed to get function pointer for " + name);
    }
    return procAddr;
  };

  // Pre-load required extension functions
  eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)getEGLExtensionProcAndCheck("eglQueryDevicesEXT");
  const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (extensions && std::string(extensions).find("EGL_EXT_device_query") != std::string::npos) {
    eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXTPROC)getEGLExtensionProcAndCheck("eglQueryDeviceStringEXT");
  }
}

void GLEngineEGL::sortAvailableDevicesByPreference(

    std::vector<int32_t>& deviceInds, EGLDeviceEXT rawDevices[]) {

  // check that we actually have the query extension
  const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (!eglQueryDeviceStringEXT) {
    info("EGL: cannot sort devices by preference, EGL_EXT_device_query is not supported");
    return;
  }

  // Build a list of devices and assign a score to each
  std::vector<std::tuple<int32_t, int32_t>> scoreDevices;
  for (int32_t iDevice : deviceInds) {
    EGLDeviceEXT device = rawDevices[iDevice];
    int score = 0;

    // Heuristic, non-software renderers seem to come last, so add a term to the score that prefers later-listed entries
    // TODO find a way to test for software rsterization for real
    score += iDevice;

    const char* vendorStrRaw = eglQueryDeviceStringEXT(device, EGL_VENDOR);

    // NOTE: on many machines (cloud VMs?) the query string above is nullptr, and this whole function does nothing
    // useful
    if (vendorStrRaw == nullptr) {
      info(5, "EGLDevice " + std::to_string(iDevice) + " -- vendor: NULL  priority score: " + std::to_string(score));
      scoreDevices.emplace_back(score, iDevice);
      continue;
    }

    std::string vendorStr = vendorStrRaw;

    // lower-case it for the checks below
    std::transform(vendorStr.begin(), vendorStr.end(), vendorStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Problem: we want to detect and prefer discrete graphics cars over integrated GPUs and
    // software / VM renderers. However, I can't figure out how to get an "is integrated"
    // property from the query device strings above. Even worse, 'AMD" and "Intel" are both
    // ambiguous and could refer to the integrated GPU or a discrete GPU.
    //
    // As a workaround, we assign scores based on the vendor: nvidia is always discrete, amd could be either, intel is
    // usually integrated, but still preferred over software renderers
    //
    // ONEDAY: figure out a better policy to detect discrete devices....

    // assign scores based on vendors to prefer discrete gpus
    const int32_t VENDOR_MULT = 100; // give this score entry a very high preference
    if (vendorStr.find("intel") != std::string::npos) score += 1 * VENDOR_MULT;
    if (vendorStr.find("amd") != std::string::npos) score += 2 * VENDOR_MULT;
    if (vendorStr.find("nvidia") != std::string::npos) score += 3 * VENDOR_MULT;

    // at high verbosity levels, log the priority
    if (polyscope::options::verbosity > 5) {
      info(5, "EGLDevice " + std::to_string(iDevice) + " -- vendor: " + vendorStr + "  priority score: " + std::to_string(score));
    }

    scoreDevices.emplace_back(score, iDevice);
  }

  // sort them by highest score
  std::sort(scoreDevices.begin(), scoreDevices.end());
  std::reverse(scoreDevices.begin(), scoreDevices.end());


  // store them back in the given array
  for (size_t i = 0; i < deviceInds.size(); i++) {
    deviceInds[i] = std::get<1>(scoreDevices[i]);
  }
}


void GLEngineEGL::initializeImGui() {

  // headless mode uses the "null" imgui backed, which essentially does nothing and just passes-through inputs to all
  // functions

  ImGui::CreateContext();
  configureImGui();
}

void GLEngineEGL::shutdown() {
  checkError();
  shutdownImGui();

  eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(eglDisplay, eglContext);
  eglTerminate(eglDisplay);
}

void GLEngineEGL::shutdownImGui() { ImGui::DestroyContext(); }

void GLEngineEGL::ImGuiNewFrame() {

  // ImGUI has an error check which fires unless we do this
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize.x = view::bufferWidth;
  io.DisplaySize.y = view::bufferHeight;

  ImGui::NewFrame();
}

void GLEngineEGL::ImGuiRender() {
  ImGui::Render();
  clearResourcesPreservedForImguiFrame();
}


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

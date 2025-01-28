// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/options.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"

#ifdef __APPLE__
#error "EGL backend is not supported on macOS. Disable it in the CMake build options."
#else
#include "glad/glad.h"
// glad must come first
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif


#include "imgui.h"

#include "polyscope/render/opengl/gl_engine.h"

#include <unordered_map>

// Note: DO NOT include this header throughout polyscope, and do not directly make openGL calls. This header should only
// be used to construct an instance of Engine. engine.h gives the render API, all render calls should pass through that.


namespace polyscope {
namespace render {
namespace backend_openGL3 {


class GLEngineEGL : public GLEngine {

public:
  GLEngineEGL();
  virtual ~GLEngineEGL();

  // High-level control
  void initialize();
  virtual void shutdown() override;
  void swapDisplayBuffers() override;
  void checkError(bool fatal = false) override;

  // EGL backend is always headless
  virtual bool isHeadless() override { return true; }

  // === Windowing and framework things

  void makeContextCurrent() override;
  void pollEvents() override;

  void focusWindow() override;
  void showWindow() override;
  void hideWindow() override;
  void updateWindowSize(bool force = false) override;
  void applyWindowSize() override;
  void setWindowResizable(bool newVal) override;
  bool getWindowResizable() override;
  std::tuple<int, int> getWindowPos() override;
  bool windowRequestsClose() override;

  bool isKeyPressed(char c) override; // for lowercase a-z and 0-9 only
  std::string getClipboardText() override;
  void setClipboardText(std::string text) override;


  // === ImGui

  void initializeImGui() override;
  void shutdownImGui() override;
  void ImGuiNewFrame() override;
  void ImGuiRender() override;

protected:
  // Function pointers for dynamic loading of EGL and extensions
  // (see explanation in resolveEGL)
  void resolveEGL();
  typedef EGLint (*eglGetErrorT)(void);
  eglGetErrorT eglGetError = nullptr;
  typedef EGLDisplay (*eglGetPlatformDisplayT)(EGLenum, void*, const EGLAttrib*);
  eglGetPlatformDisplayT eglGetPlatformDisplay = nullptr;
  typedef EGLBoolean (*eglInitializeT)(EGLDisplay, EGLint*, EGLint*);
  eglInitializeT eglInitialize = nullptr;
  typedef EGLBoolean (*eglChooseConfigT)(EGLDisplay, EGLint const*, EGLConfig*, EGLint, EGLint*);
  eglChooseConfigT eglChooseConfig = nullptr;
  typedef EGLBoolean (*eglBindAPIT)(EGLenum);
  eglBindAPIT eglBindAPI = nullptr;
  typedef EGLContext (*eglCreateContextT)(EGLDisplay, EGLConfig, EGLContext, EGLint const*);
  eglCreateContextT eglCreateContext = nullptr;
  typedef EGLBoolean (*eglMakeCurrentT)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
  eglMakeCurrentT eglMakeCurrent = nullptr;
  typedef EGLBoolean (*eglDestroyContextT)(EGLDisplay, EGLContext);
  eglDestroyContextT eglDestroyContext = nullptr;
  typedef EGLBoolean (*eglTerminateT)(EGLDisplay);
  eglTerminateT eglTerminate = nullptr;
  typedef void (*eglProcT)(void); // our helper type
  typedef eglProcT (*eglGetProcAddressT)(const char*);
  eglGetProcAddressT eglGetProcAddress = nullptr;
  typedef const char* (*eglQueryStringT)(EGLDisplay, EGLint);
  eglQueryStringT eglQueryString = nullptr;
  PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = nullptr;
  PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT = nullptr;

  // Internal windowing and engine details
  EGLDisplay eglDisplay;
  EGLContext eglContext;

  // helpers
  void checkEGLError(bool fatal = true);
  void sortAvailableDevicesByPreference(std::vector<int32_t>& deviceInds, EGLDeviceEXT rawDevices[]);
};

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

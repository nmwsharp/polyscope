// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/options.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"

#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include "GLFW/glfw3.h"
#else
#include "glad/glad.h"
// glad must come first
#include "GLFW/glfw3.h"
#endif

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "polyscope/render/opengl/gl_engine.h"

#include <unordered_map>

// Note: DO NOT include this header throughout polyscope, and do not directly make openGL calls. This header should only
// be used to construct an instance of Engine. engine.h gives the render API, all render calls should pass through that.


namespace polyscope {
namespace render {
namespace backend_openGL3 {


class GLEngineGLFW : public GLEngine {

public:
  GLEngineGLFW();
  virtual ~GLEngineGLFW();

  // High-level control
  void initialize();
  virtual void shutdown() override;
  void swapDisplayBuffers() override;

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
  // Internal windowing and engine details
  GLFWwindow* mainWindow = nullptr;
};

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

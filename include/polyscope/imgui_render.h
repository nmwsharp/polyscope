// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#else
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#endif

// This class is MOSTLY the opengl3 example from the imgui repo, however it also
// has a few tweaks for polyscope. For instance, some of the io event callback
// code is here, while the rest is in polyscope.cpp

namespace polyscope {
namespace imguirender {

// === Global state

// the primary GLFWwindow in which everthing is drawn
extern GLFWwindow* mainWindow;

// "current" time in seconds
extern double time;

// === Initilize and render the display list
bool ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks);
void ImGui_ImplGlfwGL3_NewFrame();

} // namespace imguirender
} // namespace polyscope
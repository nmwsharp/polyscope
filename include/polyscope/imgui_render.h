#pragma once

#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#endif

namespace polyscope {
namespace imgui {

// === Global state

// the primary GLFWwindow in which everthing is drawn
extern GLFWwindow* mainWindow;

// "current" time in seconds
extern double time;

// === Initilize and render the display list
bool ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks);
void ImGui_ImplGlfwGL3_NewFrame();

}  // namespace imgui
}  // namespace polyscope
cmake_minimum_required(VERSION 3.5...3.22)

project(polyscope)

### Policy settings
cmake_policy(SET CMP0054 NEW)   # don't implicitly dereference inside if()

## Project options

# Backend
set(POLYSCOPE_BACKEND_OPENGL3_GLFW "ON" CACHE BOOL "Enable openGL3_glfw backend")
set(POLYSCOPE_BACKEND_OPENGL_MOCK "ON" CACHE BOOL "Enable openGL_mock backend")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # the EGL backend is enabled by default when building on linux, and
  # disabled by default otherwise
  set(POLYSCOPE_BACKEND_OPENGL3_EGL "ON" CACHE BOOL "Enable openGL3_egl backend")
else()
  set(POLYSCOPE_BACKEND_OPENGL3_EGL "OFF" CACHE BOOL "Enable openGL3_egl backend")
endif()

### Do anything needed for dependencies and bring their stuff in to scope
add_subdirectory(deps)

### Add things for this project
add_subdirectory(src)

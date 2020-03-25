cmake_minimum_required(VERSION 2.8.9)

if("${POLYSCOPE_BACKEND}" STREQUAL "OPENGL3_GLFW")

  ## Glad
  add_subdirectory(glad)

  ## GLFW
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(glfw)

elseif("${POLYSCOPE_BACKEND}" STREQUAL "MOCK_OPENGL")
endif()

## Imgui
add_subdirectory(imgui)

## Argparse

## STB stuff
add_subdirectory(stb)

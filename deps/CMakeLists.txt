cmake_minimum_required(VERSION 2.8.9)

## Glad
add_subdirectory(glad)

## GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
target_include_directories(polyscope PRIVATE "glfw/include")

## Imgui
add_subdirectory(imgui)
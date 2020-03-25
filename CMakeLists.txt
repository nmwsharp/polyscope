cmake_minimum_required(VERSION 3.1)

project(polyscope)

### Policy settings
cmake_policy(SET CMP0054 NEW)   # don't implicitly dereference inside if()

## Project options

# Backend
set(POLYSCOPE_BACKEND "OPENGL3_GLFW" CACHE STRING "Which graphics and windowing backend to use")
set_property(CACHE POLYSCOPE_BACKEND PROPERTY STRINGS "OPENGL3_GLFW" "MOCK_OPENGL")
if("${POLYSCOPE_BACKEND}" STREQUAL "OPENGL3_GLFW")
  message("Using polyscope backend: ${POLYSCOPE_BACKEND}")
elseif("${POLYSCOPE_BACKEND}" STREQUAL "MOCK_OPENGL")
  message("Using polyscope backend: ${POLYSCOPE_BACKEND}")
else()
  message(FATAL_ERROR "Unrecognized polyscope backend from POLYSCOPE_BACKEND option, value is [${POLYSCOPE_BACKEND}]" )
endif()

### Do anything needed for dependencies and bring their stuff in to scope
add_subdirectory(deps)

### Add things for this project
add_subdirectory(src)

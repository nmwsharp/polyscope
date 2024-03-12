cmake_minimum_required(VERSION 3.5...3.22)

project(polyscope)

# Detects whether this is a top-level project
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(POLYSCOPE_TOPLEVEL_PROJECT OFF)
else()
    set(POLYSCOPE_TOPLEVEL_PROJECT ON)
endif()

### Policy settings
cmake_policy(SET CMP0054 NEW)   # don't implicitly dereference inside if()

## Project options

# Backend
set(POLYSCOPE_BACKEND_OPENGL3_GLFW "ON" CACHE BOOL "Enable openGL3_glfw backend")
set(POLYSCOPE_BACKEND_OPENGL_MOCK "ON" CACHE BOOL "Enable openGL_mock backend")

### Do anything needed for dependencies and bring their stuff in to scope
add_subdirectory(deps)

### Add things for this project
add_subdirectory(src)

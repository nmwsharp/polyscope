cmake_minimum_required(VERSION 3.1)

project(polyscope)

### Policy settings
cmake_policy(SET CMP0054 NEW)   # don't implicitly dereference inside if()

### Do anything needed for dependencies and bring their stuff in to scope
add_subdirectory(deps)

### Add things for this project
add_subdirectory(src)

### Build the demo viewer application
option(BUILD_POLYSCOPE_VIEWER "Build the viewer app." OFF)
if(BUILD_POLYSCOPE_VIEWER)
    add_subdirectory(src/viewer-app)
endif()

option(BUILD_POLYSCOPE_DEMO "Build the demo app." OFF)
if(BUILD_POLYSCOPE_DEMO)
    add_subdirectory(src/demo-app)
endif()



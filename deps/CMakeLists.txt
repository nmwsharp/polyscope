cmake_minimum_required(VERSION 2.8.9)

## Glad
add_subdirectory(glad)

## GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

## Imgui
add_subdirectory(imgui)

## Geometry Central
add_subdirectory(geometry-central)
set(GEOMETRY_CENTRAL_INCLUDE_DIRS ${GEOMETRY_CENTRAL_INCLUDE_DIRS} PARENT_SCOPE)

if(HAVE_SUITESPARSE)
    SET(HAVE_SUITESPARSE TRUE PARENT_SCOPE)
endif()

## Argparse


cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

set(SRCS imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_demo.cpp imgui/examples/imgui_impl_glfw.cpp imgui/examples/imgui_impl_opengl3.cpp)

add_library(
        imgui
        ${SRCS}
        )

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include/")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glad/include/")


add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

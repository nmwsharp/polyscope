cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

set(HEADS imgui/imgui.h imgui/imconfig.h imgui/stb_rect_pack.h imgui/stb_textedit.h imgui/stb_truetype.h)
set(SRCS imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_demo.cpp)

add_library(
        imgui
        ${SRCS}
        ${HEADS}
        )
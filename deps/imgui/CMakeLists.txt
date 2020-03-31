cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()


# Append Wno-error to avoid compiling imgui with Werror; imgui seems to trigger lots of warnings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
endif()


if("${POLYSCOPE_BACKEND_OPENGL3_GLFW}")

  set(SRCS imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_demo.cpp imgui/examples/imgui_impl_glfw.cpp imgui/examples/imgui_impl_opengl3.cpp)

  add_library(
          imgui
          ${SRCS}
          )

  target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")
  target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include/")
  target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glad/include/")

  if(APPLE)
    add_definitions(-DGLFW_INCLUDE_GLCOREARB)
    add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM=<GLFW/glfw3.h>)
  else()
    add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
  endif()

elseif("${POLYSCOPE_BACKEND_OPENGL_MOCK}")

  # Disable every platform-specific thing I can find in imgui
  add_definitions(-DIMGUI_DISABLE_OSX_FUNCTIONS)
  add_definitions(-DIMGUI_DISABLE_WIN32_FUNCTIONS)
  add_definitions(-DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)
  add_definitions(-DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
  
  set(SRCS imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp)

  add_library(
          imgui
          ${SRCS}
          )

  target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

endif()



set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

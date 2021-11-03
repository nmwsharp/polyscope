if("${POLYSCOPE_BACKEND_OPENGL3_GLFW}")

  ## Glad
  add_subdirectory(glad)

  ## GLFW
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(glfw)
endif()

if("${POLYSCOPE_BACKEND_OPENGL_MOCK}")
endif()

## Imgui
add_subdirectory(imgui)

## Argparse

## Other dependencies
add_subdirectory(stb)
add_subdirectory(MarchingCubeCpp)

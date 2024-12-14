if("${POLYSCOPE_BACKEND_OPENGL3_GLFW}")
  # # Glad
  if(NOT TARGET glad)
    add_subdirectory(glad)
    set_target_properties(glad PROPERTIES ${OUTPUT_DIR})
  endif()

  # # GLFW
  # if(NOT TARGET glfw)
  # set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  # set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  # set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  # set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  # add_subdirectory(glfw)
  # endif()
endif()

# # glm
# if(NOT TARGET glm::glm)
# add_subdirectory(glm)
# target_compile_definitions(glm INTERFACE GLM_ENABLE_EXPERIMENTAL)
# set_target_properties(glm PROPERTIES LINKER_LANGUAGE CXX)
# endif()

# # Imgui
# if(NOT TARGET imgui)
# add_subdirectory(imgui)
# endif()

# # Json
if(NOT TARGET nlohmann_json::nlohmann_json)
  add_subdirectory(json)
endif()

# # MarchingCube
if(NOT TARGET MarchingCube::MarchingCube)
  add_subdirectory(MarchingCubeCpp)
endif()

# # stb
if(NOT TARGET stb_image)
  add_subdirectory(stb_image)
endif()

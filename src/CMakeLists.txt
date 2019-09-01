cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

SET(SRCS

  # Core functionality 
  polyscope.cpp
  screenshot.cpp
  structure.cpp
  utilities.cpp
  view.cpp
  messages.cpp
  pick.cpp

  # General utilities
  disjoint_sets.cpp
  file_helpers.cpp
  camera_parameters.cpp
  histogram.cpp

  # Rendering stuff
  gl/gl_utils.cpp
  gl/ground_plane.cpp
  gl/color_maps.cpp
  
  color_management.cpp

  gl/materials/bindata_concrete_seamless.cpp
  gl/materials/bindata_mat_wax.cpp
  gl/materials/materials.cpp

  binary_font_cousine_regular.cpp

  # Rendering utilities
  trace_vector_field.cpp
  ribbon_artist.cpp

  ## Structures

  # Point cloud
  point_cloud.cpp
  point_cloud_color_quantity.cpp
  point_cloud_scalar_quantity.cpp
  point_cloud_vector_quantity.cpp

  # Surface
  surface_mesh.cpp
  surface_mesh_io.cpp
  surface_scalar_quantity.cpp
  surface_color_quantity.cpp
  surface_distance_quantity.cpp
  surface_vector_quantity.cpp
  surface_count_quantity.cpp
  surface_graph_quantity.cpp
  surface_parameterization_quantity.cpp
  #surface_subset_quantity.cpp
  #surface_selection_quantity.cpp
  #surface_input_curve_quantity.cpp
  
  # Curve network
  curve_network.cpp
  curve_network_scalar_quantity.cpp
  curve_network_color_quantity.cpp
  curve_network_vector_quantity.cpp
)


SET(INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include/polyscope/")
SET(HEADERS
  ${INCLUDE_ROOT}/affine_remapper.h
  ${INCLUDE_ROOT}/affine_remapper.ipp
  ${INCLUDE_ROOT}/camera_parameters.h
  ${INCLUDE_ROOT}/color_management.h
  ${INCLUDE_ROOT}/combining_hash_functions.h
  ${INCLUDE_ROOT}/curve_network.h
  ${INCLUDE_ROOT}/curve_network.ipp
  ${INCLUDE_ROOT}/curve_network_color_quantity.h
  ${INCLUDE_ROOT}/curve_network_quantity.h
  ${INCLUDE_ROOT}/curve_network_scalar_quantity.h
  ${INCLUDE_ROOT}/curve_network_vector_quantity.h
  ${INCLUDE_ROOT}/disjoint_sets.h
  ${INCLUDE_ROOT}/file_helpers.h
  ${INCLUDE_ROOT}/histogram.h
  ${INCLUDE_ROOT}/imgui_render.h
  ${INCLUDE_ROOT}/messages.h
  ${INCLUDE_ROOT}/options.h
  ${INCLUDE_ROOT}/pick.h
  ${INCLUDE_ROOT}/point_cloud.h
  ${INCLUDE_ROOT}/point_cloud.ipp
  ${INCLUDE_ROOT}/point_cloud_color_quantity.h
  ${INCLUDE_ROOT}/point_cloud_quantity.h
  ${INCLUDE_ROOT}/point_cloud_scalar_quantity.h
  ${INCLUDE_ROOT}/point_cloud_vector_quantity.h
  ${INCLUDE_ROOT}/polyscope.h
  ${INCLUDE_ROOT}/quantity.h
  ${INCLUDE_ROOT}/quantity.ipp
  ${INCLUDE_ROOT}/ribbon_artist.h
  ${INCLUDE_ROOT}/screenshot.h
  ${INCLUDE_ROOT}/standardize_data_array.h
  ${INCLUDE_ROOT}/structure.h
  ${INCLUDE_ROOT}/structure.ipp
  ${INCLUDE_ROOT}/surface_color_quantity.h
  ${INCLUDE_ROOT}/surface_count_quantity.h
  ${INCLUDE_ROOT}/surface_distance_quantity.h
  ${INCLUDE_ROOT}/surface_graph_quantity.h
  ${INCLUDE_ROOT}/surface_input_curve_quantity.h
  ${INCLUDE_ROOT}/surface_mesh.h
  ${INCLUDE_ROOT}/surface_mesh.ipp
  ${INCLUDE_ROOT}/surface_mesh_enums.h
  ${INCLUDE_ROOT}/surface_mesh_io.h
  ${INCLUDE_ROOT}/surface_mesh_quantity.h
  ${INCLUDE_ROOT}/surface_parameterization_enums.h
  ${INCLUDE_ROOT}/surface_parameterization_quantity.h
  ${INCLUDE_ROOT}/surface_scalar_quantity.h
  ${INCLUDE_ROOT}/surface_selection_quantity.h
  ${INCLUDE_ROOT}/surface_subset_quantity.h
  ${INCLUDE_ROOT}/surface_vector_quantity.h
  ${INCLUDE_ROOT}/trace_vector_field.h
  ${INCLUDE_ROOT}/utilities.h
  ${INCLUDE_ROOT}/view.h
  ${INCLUDE_ROOT}/gl/colors.h
  ${INCLUDE_ROOT}/gl/color_maps.h
  ${INCLUDE_ROOT}/gl/gl_utils.h
  ${INCLUDE_ROOT}/gl/ground_plane.h
  ${INCLUDE_ROOT}/gl/shaders.h
  ${INCLUDE_ROOT}/gl/materials/materials.h
  ${INCLUDE_ROOT}/gl/shaders/common.h
  ${INCLUDE_ROOT}/gl/shaders/cylinder_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/distance_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/ground_plane_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/histogram_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/image_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/parameterization_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/ray_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/ribbon_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/sphere_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/surface_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/texture_draw_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/vector_shaders.h
  ${INCLUDE_ROOT}/gl/shaders/wireframe_shaders.h
)

# Create a single library for the project
add_library(polyscope ${SRCS} ${HEADERS})

# Required compiler settings
set_property(TARGET polyscope PROPERTY CXX_STANDARD 11)
set_property(TARGET polyscope PROPERTY CXX_STANDARD_REQUIRED TRUE)
add_definitions(-DNOMINMAX)


# Include settings
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/args")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/glad/include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/glfw/include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/glm")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/happly")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/stb")
        
# Link settings
target_link_libraries(polyscope imgui glfw ${GLFW_LIBRARIES} stb)

if(APPLE)
    # Apple is playing hardball and deprecating openGL, we'll see how that goes
    # Silence warnings about openGL deprecation
    add_definitions (-DGL_SILENCE_DEPRECATION)  

    find_library(cocoa_library Cocoa)
    find_library(opengl_library OpenGL)
    find_library(corevideo_library CoreVideo)
    find_library(iokit_library IOKit)
    target_link_libraries(polyscope ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
else()
    target_link_libraries(polyscope glad)
endif()

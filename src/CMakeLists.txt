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


# Create a single library for the project
add_library(polyscope ${SRCS})

# Required compiler settings
set_property(TARGET polyscope PROPERTY CXX_STANDARD 11)
set_property(TARGET polyscope PROPERTY CXX_STANDARD_REQUIRED TRUE)


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

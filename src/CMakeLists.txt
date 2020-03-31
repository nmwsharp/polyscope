cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

SET(INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include/polyscope/")

# Lists that will be populated for the render backend

# Add the main _engine file no matter what. All of the interesting parts are ifdef'd out, and this
# allows us to resolve stubs.
list (APPEND BACKEND_SRCS
		render/opengl/gl_engine.cpp  
		render/mock_opengl/mock_gl_engine.cpp  
)

# Configure the render backend
if("${POLYSCOPE_BACKEND_OPENGL3_GLFW}")
  message("Polyscope backend openGL3_glfw enabled")
	
  list (APPEND BACKEND_SRCS
		render/opengl/gl_engine.cpp  
		render/opengl/shaders/texture_draw_shaders.cpp  
		render/opengl/shaders/lighting_shaders.cpp  
		render/opengl/shaders/ground_plane_shaders.cpp  
		render/opengl/shaders/histogram_shaders.cpp  
		render/opengl/shaders/surface_mesh_shaders.cpp  
		render/opengl/shaders/vector_shaders.cpp  
		render/opengl/shaders/sphere_shaders.cpp  
		render/opengl/shaders/ribbon_shaders.cpp  
		render/opengl/shaders/cylinder_shaders.cpp  
	)

  list(APPEND BACKEND_HEADERS
		${INCLUDE_ROOT}render/opengl/gl_engine.h
		${INCLUDE_ROOT}render/opengl/shaders/common.h
	)

  list(APPEND BACKEND_INCLUDE_DIRS
		${CMAKE_CURRENT_SOURCE_DIR}/../deps/glad/include
		${CMAKE_CURRENT_SOURCE_DIR}/../deps/glfw/include
	)

	# Link settings
  list(APPEND BACKEND_LIBS
		glfw ${GLFW_LIBRARIES}
	)

	if(APPLE)
			# Apple is playing hardball and deprecating openGL... we'll cross that bridge when we come to it
			# Silence warnings about openGL deprecation
			add_definitions(-DGL_SILENCE_DEPRECATION)  

			find_library(cocoa_library Cocoa)
			find_library(opengl_library OpenGL)
			find_library(corevideo_library CoreVideo)
			find_library(iokit_library IOKit)
			list(APPEND BACKEND_LIBS ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
	else()
			list(APPEND BACKEND_LIBS glad)
	endif()
			
  add_definitions(-DPOLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED)  
endif()
	
if("${POLYSCOPE_BACKEND_OPENGL_MOCK}")
  message("Polyscope backend openGL_mock enabled")
	
  list (APPEND BACKEND_SRCS
		render/mock_opengl/mock_gl_engine.cpp  
		render/opengl/shaders/texture_draw_shaders.cpp  
		render/opengl/shaders/lighting_shaders.cpp  
		render/opengl/shaders/ground_plane_shaders.cpp  
		render/opengl/shaders/histogram_shaders.cpp  
		render/opengl/shaders/surface_mesh_shaders.cpp  
		render/opengl/shaders/vector_shaders.cpp  
		render/opengl/shaders/sphere_shaders.cpp  
		render/opengl/shaders/ribbon_shaders.cpp  
		render/opengl/shaders/cylinder_shaders.cpp  
	)

  list(APPEND BACKEND_HEADERS
		${INCLUDE_ROOT}render/mock_opengl/mock_gl_engine.h
		${INCLUDE_ROOT}render/opengl/shaders/common.h
	)

  list(APPEND BACKEND_INCLUDE_DIRS
	)

	# Link settings
  list(APPEND BACKEND_LIBS
	)

add_definitions(-DPOLYSCOPE_BACKEND_OPENGL_MOCK_ENABLED)  
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
  
	# Rendering stuff
  render/engine.cpp  
  render/color_maps.cpp
  render/ground_plane.cpp
  render/materials.cpp
  render/initialize_backend.cpp  

  # General utilities
  disjoint_sets.cpp
  file_helpers.cpp
  camera_parameters.cpp
  histogram.cpp
  persistent_value.cpp
  color_management.cpp

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
	surface_parameterization_quantity.cpp
	surface_vector_quantity.cpp
	surface_count_quantity.cpp
	surface_graph_quantity.cpp
  #surface_subset_quantity.cpp
  #surface_selection_quantity.cpp
  #surface_input_curve_quantity.cpp
  
  # Curve network
	curve_network.cpp
	curve_network_scalar_quantity.cpp
	curve_network_color_quantity.cpp
	curve_network_vector_quantity.cpp
  
  # Rendering utilities
	trace_vector_field.cpp
	ribbon_artist.cpp
	
	## Embedded binary data 
  render/bindata/bindata_font_cousine_regular.cpp
  render/bindata/concrete_seamless.cpp
  render/bindata/bindata_clay.cpp
  render/bindata/bindata_wax.cpp
  render/bindata/bindata_candy.cpp
  render/bindata/bindata_flat.cpp
  render/bindata/bindata_mud.cpp
  render/bindata/bindata_ceramic.cpp
  render/bindata/bindata_jade.cpp
  render/bindata/bindata_normal.cpp
)


# Setting headers is useful for some build systems. For instance, in Visual Studio this is necessary for the header files to be indexed as a part of the project.
SET(HEADERS
	${INCLUDE_ROOT}/affine_remapper.h
	${INCLUDE_ROOT}/affine_remapper.ipp
	${INCLUDE_ROOT}/camera_parameters.h
	${INCLUDE_ROOT}/color_management.h
	${INCLUDE_ROOT}/colors.h
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
	${INCLUDE_ROOT}/messages.h
	${INCLUDE_ROOT}/options.h
	${INCLUDE_ROOT}/persistent_value.h
	${INCLUDE_ROOT}/pick.h
	${INCLUDE_ROOT}/pick.ipp
	${INCLUDE_ROOT}/point_cloud.h
	${INCLUDE_ROOT}/point_cloud.ipp
	${INCLUDE_ROOT}/point_cloud_color_quantity.h
	${INCLUDE_ROOT}/point_cloud_quantity.h
	${INCLUDE_ROOT}/point_cloud_scalar_quantity.h
	${INCLUDE_ROOT}/point_cloud_vector_quantity.h
	${INCLUDE_ROOT}/polyscope.h
	${INCLUDE_ROOT}/quantity.h
	${INCLUDE_ROOT}/quantity.ipp
	${INCLUDE_ROOT}/render/color_maps.h
	${INCLUDE_ROOT}/render/engine.h
	${INCLUDE_ROOT}/render/engine.ipp
	${INCLUDE_ROOT}/render/ground_plane.h
	${INCLUDE_ROOT}/render/material_defs.h
	${INCLUDE_ROOT}/render/materials.h
	${INCLUDE_ROOT}/render/shaders.h
	${INCLUDE_ROOT}/ribbon_artist.h
	${INCLUDE_ROOT}/scaled_value.h
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
)

# Create a single library for the project
add_library(polyscope ${SRCS} ${BACKEND_SRCS} ${HEADERS} ${BACKEND_HEADERS})

# Required compiler settings
set_property(TARGET polyscope PROPERTY CXX_STANDARD 11)
set_property(TARGET polyscope PROPERTY CXX_STANDARD_REQUIRED TRUE)
set_target_properties(polyscope PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_definitions(-DNOMINMAX)

# Include settings
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/args")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/glm")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/happly")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/json/include")
target_include_directories(polyscope PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../deps/stb")
target_include_directories(polyscope PUBLIC "${BACKEND_INCLUDE_DIRS}")
        
# Link settings
target_link_libraries(polyscope "${BACKEND_LIBS}" imgui stb)

cmake_minimum_required(VERSION 3.5)

# Create a library for the viewer code
add_library(
    glad
    glad.c
    )

# Are we building a shared library?
get_target_property(library_type glad TYPE)
if (library_type STREQUAL SHARED_LIBRARY)
  # If so, define the macro GLAD_API_EXPORT on the command line.
  target_compile_definitions(glad PUBLIC GLAD_GLAPI_EXPORT PRIVATE GLAD_GLAPI_EXPORT_BUILD)
endif()

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>)

set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(TARGETS glad EXPORT ${EXPORTED_TARGETS_NAME} DESTINATION "${INSTALL_LIB_DIR}")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include DESTINATION "${INSTALL_INCLUDE_DIR}")

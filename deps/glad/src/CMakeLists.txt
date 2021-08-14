cmake_minimum_required(VERSION 3.1)

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

target_include_directories(glad PRIVATE "../include")    

set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

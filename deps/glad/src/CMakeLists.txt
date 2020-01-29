cmake_minimum_required(VERSION 2.8.9)

# Create a library for the viewer code
add_library(
    glad
    glad.c
    )

target_include_directories(glad PRIVATE "../include")    

set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

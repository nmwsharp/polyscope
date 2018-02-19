cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

# Create a single library for the project
add_executable(
        polyscope_basic_demo 
        main.cpp
        # add any additional source files here
        )

# Link settings
target_link_libraries(polyscope_basic_demo polyscope)

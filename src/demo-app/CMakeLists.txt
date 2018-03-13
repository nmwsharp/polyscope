cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()


# Create a single library for the project
add_executable(
        polyscopedemo
        demo_app.cpp
        )

# Link settings
target_link_libraries(polyscopedemo polyscope)

cmake_minimum_required(VERSION 2.8.9)

# Don't build polyscope demo app
set(BUILD_POLYSCOPE_VIEWER "FALSE")
add_subdirectory(polyscope)

if(HAVE_SUITESPARSE)
    SET(HAVE_SUITESPARSE TRUE PARENT_SCOPE)
endif()

cmake_minimum_required(VERSION 2.8.9)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

# Collect all .cpp files in the subdirectories
file(GLOB SRCS *.cpp)
file(GLOB_RECURSE GL_SRCS gl/*.cpp)
file(GLOB_RECURSE HEADS ../include/*.h ../include/*.hpp ../include/*.ipp)

# Create a single library for the project
add_library(
        polyscope 
        ${SRCS}
        ${GL_SRCS}
        ${HEADS}
        )

# Include settings
target_include_directories(polyscope PUBLIC "../include")
target_include_directories(polyscope PUBLIC "../deps/glfw/include")
target_include_directories(polyscope PRIVATE "../deps/glad/include")
target_include_directories(polyscope PRIVATE "../deps/glad/include")
target_include_directories(polyscope PUBLIC ${GEOMETRY_CENTRAL_INCLUDE_DIRS})
target_include_directories(polyscope PUBLIC "../deps/imgui/imgui")
        
# Link settings
target_link_libraries(polyscope imgui glfw ${GLFW_LIBRARIES} geometry-central)

if(APPLE)
    find_library(cocoa_library Cocoa)
    find_library(opengl_library OpenGL)
    find_library(corevideo_library CoreVideo)
    find_library(iokit_library IOKit)
    target_link_libraries(polyscope ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
else()
    target_link_libraries(polyscope glad)
endif()
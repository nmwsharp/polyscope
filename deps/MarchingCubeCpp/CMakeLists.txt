cmake_minimum_required(VERSION 3.1)

# Create a library for the viewer code
add_library(
    MarchingCube 
    src/MC_impl.cpp
    )

target_include_directories(MarchingCube PRIVATE "include")    
target_include_directories(MarchingCube PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../glm")

set_target_properties(MarchingCube PROPERTIES 
                                    POSITION_INDEPENDENT_CODE TRUE
                                    CXX_STANDARD 11
                                    CXX_STANDARD_REQUIRED TRUE
                                   )

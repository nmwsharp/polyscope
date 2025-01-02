add_library(MarchingCube INTERFACE)
add_library(MarchingCube::MarchingCube ALIAS MarchingCube)

target_include_directories(MarchingCube INTERFACE include)

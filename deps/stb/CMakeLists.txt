# Create a library for the viewer code
add_library(
    stb
    stb_impl.cpp
)

target_include_directories(stb PUBLIC ".")

set_target_properties(stb PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Create a library for the viewer code
add_library(
    stb_image
    STATIC
    stb_impl.cpp
)

target_include_directories(stb_image PUBLIC ".")

set_target_properties(stb_image PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

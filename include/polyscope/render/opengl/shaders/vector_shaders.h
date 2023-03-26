// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification FLEX_VECTOR_VERT_SHADER;
extern const ShaderStageSpecification FLEX_TANGENT_VECTOR_VERT_SHADER;
extern const ShaderStageSpecification FLEX_VECTOR_GEOM_SHADER;
extern const ShaderStageSpecification FLEX_VECTOR_FRAG_SHADER;

// Rules specific to cylinders
extern const ShaderReplacementRule VECTOR_PROPAGATE_COLOR;
extern const ShaderReplacementRule VECTOR_CULLPOS_FROM_TAIL;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

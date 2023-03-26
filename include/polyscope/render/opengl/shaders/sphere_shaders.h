// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification FLEX_SPHERE_VERT_SHADER;
extern const ShaderStageSpecification FLEX_SPHERE_GEOM_SHADER;
extern const ShaderStageSpecification FLEX_SPHERE_FRAG_SHADER;

extern const ShaderStageSpecification FLEX_POINTQUAD_VERT_SHADER;
extern const ShaderStageSpecification FLEX_POINTQUAD_GEOM_SHADER;
extern const ShaderStageSpecification FLEX_POINTQUAD_FRAG_SHADER;

// Rules specific to spheres
extern const ShaderReplacementRule SPHERE_PROPAGATE_VALUE;
extern const ShaderReplacementRule SPHERE_PROPAGATE_VALUE2;
extern const ShaderReplacementRule SPHERE_PROPAGATE_COLOR;
extern const ShaderReplacementRule SPHERE_VARIABLE_SIZE;
extern const ShaderReplacementRule SPHERE_CULLPOS_FROM_CENTER;
extern const ShaderReplacementRule SPHERE_CULLPOS_FROM_CENTER_QUAD;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification RIBBON_VERT_SHADER;
extern const ShaderStageSpecification RIBBON_GEOM_SHADER;
extern const ShaderStageSpecification RIBBON_FRAG_SHADER;

// Rules
// extern const ShaderReplacementRule RULE_NAME;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

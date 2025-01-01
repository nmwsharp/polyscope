// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

// High level pipeline
extern const ShaderStageSpecification HISTOGRAM_VERT_SHADER;
extern const ShaderStageSpecification HISTOGRAM_FRAG_SHADER;
extern const ShaderStageSpecification HISTOGRAM_CATEGORICAL_FRAG_SHADER;

// Rules
// extern const ShaderReplacementRule RULE_NAME;

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

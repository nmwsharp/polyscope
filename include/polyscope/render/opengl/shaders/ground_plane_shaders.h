// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

extern const ShaderStageSpecification GROUND_PLANE_VERT_SHADER;
extern const ShaderStageSpecification GROUND_PLANE_TILE_FRAG_SHADER;
extern const ShaderStageSpecification GROUND_PLANE_TILE_REFLECT_FRAG_SHADER;
extern const ShaderStageSpecification GROUND_PLANE_SHADOW_FRAG_SHADER;

// Rules
// extern const ShaderReplacementRule RULE_NAME;

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

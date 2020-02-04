#pragma once

#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

// == Texture draw shaders
extern const ShaderStageSpecification TEXTURE_DRAW_VERT_SHADER;
extern const ShaderStageSpecification PLAIN_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification MAP3_TEXTURE_DRAW_FRAG_SHADER;

// == Surface mesh shaders
extern const ShaderStageSpecification PLAIN_SURFACE_VERT_SHADER;
extern const ShaderStageSpecification PLAIN_SURFACE_FRAG_SHADER;


} // namespace render
} // namespace polyscope

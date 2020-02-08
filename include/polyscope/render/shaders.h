#pragma once

#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

// == Texture draw shaders
extern const ShaderStageSpecification TEXTURE_DRAW_VERT_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_VERT_SHADER;
extern const ShaderStageSpecification PLAIN_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification MAP3_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_FRAG_SHADER;

// == Lighting shaders
extern const ShaderStageSpecification MAP_LIGHT_FRAG_SHADER;
extern const ShaderStageSpecification SPLIT_SPECULAR_PRECOMPUTE_FRAG_SHADER;

// == Histogram shaders
extern const ShaderStageSpecification HISTOGRAM_VERT_SHADER;
extern const ShaderStageSpecification HISTOGRAM_FRAG_SHADER;

// == Ground plane shaders
extern const ShaderStageSpecification GROUND_PLANE_VERT_SHADER;
extern const ShaderStageSpecification GROUND_PLANE_FRAG_SHADER;

// == Surface mesh shaders
extern const ShaderStageSpecification PLAIN_SURFACE_VERT_SHADER;
extern const ShaderStageSpecification PLAIN_SURFACE_FRAG_SHADER;
extern const ShaderStageSpecification VERTCOLOR3_SURFACE_VERT_SHADER;
extern const ShaderStageSpecification VERTCOLOR3_SURFACE_FRAG_SHADER;
extern const ShaderStageSpecification VERTCOLOR_SURFACE_VERT_SHADER;
extern const ShaderStageSpecification VERTCOLOR_SURFACE_FRAG_SHADER;
extern const ShaderStageSpecification HALFEDGECOLOR_SURFACE_VERT_SHADER;
extern const ShaderStageSpecification HALFEDGECOLOR_SURFACE_FRAG_SHADER;


} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

extern const ShaderStageSpecification TEXTURE_DRAW_VERT_SHADER;
extern const ShaderStageSpecification TEXTURE_DRAW_UPPERLEFT_VERT_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_VERT_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification PLAIN_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification PLAIN_RENDERIMAGE_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification MAP3_TEXTURE_DRAW_FRAG_SHADER;
extern const ShaderStageSpecification COMPOSITE_PEEL;
extern const ShaderStageSpecification DEPTH_COPY;
extern const ShaderStageSpecification DEPTH_TO_MASK;
extern const ShaderStageSpecification BLUR_RGB;

extern const ShaderStageSpecification SCALAR_TEXTURE_COLORMAP;


// Rules
// extern const ShaderReplacementRule RULE_NAME;
extern const ShaderReplacementRule
    TEXTURE_ORIGIN_UPPERLEFT; // sample textures with (0,0) in the upper left, instead of the usual openGL lower left
extern const ShaderReplacementRule
    TEXTURE_ORIGIN_LOWERLEFT; // sample textures with (0,0) in the lower left, which is the usual openGL rule
extern const ShaderReplacementRule TEXTURE_SET_TRANSPARENCY; // apply a transparency uniform to the texture
extern const ShaderReplacementRule TEXTURE_SHADE_COLOR;      // sample a color from a texture and use it for shading
extern const ShaderReplacementRule TEXTURE_PROPAGATE_VALUE;  // sample a scalar from a texture and use it for shading
extern const ShaderReplacementRule
    TEXTURE_BILLBOARD_FROM_UNIFORMS; // adjust a texture's billboard position via uniforms


// Shaders (which are used elsewhere)
extern const ShaderStageSpecification TEXTURE_DRAW_VERT_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_VERT_SHADER;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

// High level pipeline
extern const ShaderStageSpecification MAP_LIGHT_FRAG_SHADER;

// Rules
extern const ShaderReplacementRule DOWNSAMPLE_RESOLVE_1;
extern const ShaderReplacementRule DOWNSAMPLE_RESOLVE_2;
extern const ShaderReplacementRule DOWNSAMPLE_RESOLVE_3;
extern const ShaderReplacementRule DOWNSAMPLE_RESOLVE_4;

extern const ShaderReplacementRule INVERSE_TONEMAP;

extern const ShaderReplacementRule TRANSPARENCY_RESOLVE_SIMPLE;
extern const ShaderReplacementRule TRANSPARENCY_STRUCTURE;
extern const ShaderReplacementRule TRANSPARENCY_PEEL_STRUCTURE;
extern const ShaderReplacementRule TRANSPARENCY_PEEL_GROUND;

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification SLICE_TETS_VERT_SHADER;
extern const ShaderStageSpecification SLICE_TETS_GEOM_SHADER;
extern const ShaderStageSpecification SLICE_TETS_FRAG_SHADER;
extern const ShaderReplacementRule SLICE_TETS_BASECOLOR_SHADE;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

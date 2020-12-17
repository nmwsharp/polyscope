#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification HISTOGRAM_VERT_SHADER;
extern const ShaderStageSpecification HISTOGRAM_FRAG_SHADER;

// Rules
//extern const ShaderReplacementRule RULE_NAME;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const std::vector<ShaderStageSpecification> TEXTURE_DRAW_PLAIN_PIPELINE;
extern const std::vector<ShaderStageSpecification> TEXTURE_DRAW_DOT3_PIPELINE;
extern const std::vector<ShaderStageSpecification> TEXTURE_DRAW_MAP3_PIPELINE;
extern const std::vector<ShaderStageSpecification> TEXTURE_DRAW_SPHEREBG_PIPELINE;


// Rules
// extern const ShaderReplacementRule RULE_NAME;

// Shaders (which are used elsewhere)
extern const ShaderStageSpecification TEXTURE_DRAW_VERT_SHADER;
extern const ShaderStageSpecification SPHEREBG_DRAW_VERT_SHADER;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

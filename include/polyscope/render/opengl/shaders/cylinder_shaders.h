#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const std::vector<ShaderStageSpecification> RAYCAST_CYLINDER_PIPELINE;

// Rules specific to cylinders
extern const ShaderReplacementRule CYLINDER_PROPAGATE_VALUE;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_VALUE;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_COLOR;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_COLOR;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_PICK;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

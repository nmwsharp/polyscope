#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const std::vector<ShaderStageSpecification> RAYCAST_SPHERE_PIPELINE;

// Rules specific to spheres
extern const ShaderReplacementRule SPHERE_PROPAGATE_VALUE;
extern const ShaderReplacementRule SPHERE_PROPAGATE_COLOR;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification FLEX_CYLINDER_VERT_SHADER;
extern const ShaderStageSpecification FLEX_CYLINDER_GEOM_SHADER;
extern const ShaderStageSpecification FLEX_CYLINDER_FRAG_SHADER;

// Rules specific to cylinders
extern const ShaderReplacementRule CYLINDER_PROPAGATE_VALUE;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_VALUE;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_COLOR;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_COLOR;
extern const ShaderReplacementRule CYLINDER_PROPAGATE_PICK;
extern const ShaderReplacementRule CYLINDER_CULLPOS_FROM_MID;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

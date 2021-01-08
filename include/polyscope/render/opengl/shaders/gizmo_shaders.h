#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// Stages
extern const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_VERT;
extern const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_FRAG;

// Rules 
extern const ShaderReplacementRule TRANSFORMATION_GIZMO_VEC;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

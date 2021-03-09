#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const ShaderStageSpecification FLEX_MESH_VERT_SHADER;
extern const ShaderStageSpecification FLEX_MESH_FRAG_SHADER;

// Rules specific to meshes 
extern const ShaderReplacementRule MESH_WIREFRAME;
extern const ShaderReplacementRule MESH_BACKFACE_NORMAL_FLIP;
extern const ShaderReplacementRule MESH_BACKFACE_DARKEN;
extern const ShaderReplacementRule MESH_PROPAGATE_VALUE;
extern const ShaderReplacementRule MESH_PROPAGATE_VALUE2;
extern const ShaderReplacementRule MESH_PROPAGATE_COLOR;
extern const ShaderReplacementRule MESH_PROPAGATE_HALFEDGE_VALUE;
extern const ShaderReplacementRule MESH_PROPAGATE_CULLPOS;
extern const ShaderReplacementRule MESH_PROPAGATE_PICK;
extern const ShaderReplacementRule MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

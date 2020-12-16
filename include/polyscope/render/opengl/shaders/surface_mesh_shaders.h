#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// High level pipeline
extern const std::vector<ShaderStageSpecification> MESH_PIPELINE;

// Rules specific to meshes 
extern const ShaderReplacementRule MESH_WIREFRAME;
extern const ShaderReplacementRule MESH_PROPAGATE_VALUE;
extern const ShaderReplacementRule MESH_PROPAGATE_VALUE2;
extern const ShaderReplacementRule MESH_PROPAGATE_COLOR;
extern const ShaderReplacementRule MESH_PROPAGATE_HALFEDGE_VALUE;
extern const ShaderReplacementRule MESH_PROPAGATE_PICK;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

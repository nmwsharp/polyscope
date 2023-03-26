// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// Stages
extern const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_VERT;
extern const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_FRAG;
extern const ShaderStageSpecification SLICE_PLANE_VERT_SHADER;
extern const ShaderStageSpecification SLICE_PLANE_FRAG_SHADER;

// Rules
extern const ShaderReplacementRule TRANSFORMATION_GIZMO_VEC;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

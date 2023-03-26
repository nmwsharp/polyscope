// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
extern const ShaderReplacementRule SLICE_TETS_MESH_WIREFRAME;
extern const ShaderReplacementRule SLICE_TETS_PROPAGATE_VALUE;
extern const ShaderReplacementRule SLICE_TETS_PROPAGATE_VECTOR;
extern const ShaderReplacementRule SLICE_TETS_VECTOR_COLOR;


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

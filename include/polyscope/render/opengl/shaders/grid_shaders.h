// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

// High level pipeline
extern const ShaderStageSpecification FLEX_GRIDCUBE_VERT_SHADER;
extern const ShaderStageSpecification FLEX_GRIDCUBE_GEOM_SHADER;
extern const ShaderStageSpecification FLEX_GRIDCUBE_FRAG_SHADER;

extern const ShaderStageSpecification FLEX_GRIDCUBE_PLANE_VERT_SHADER;
extern const ShaderStageSpecification FLEX_GRIDCUBE_PLANE_FRAG_SHADER;

// Rules
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_NODE_VALUE;
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_CELL_VALUE;
extern const ShaderReplacementRule GRIDCUBE_WIREFRAME;
extern const ShaderReplacementRule GRIDCUBE_PLANE_WIREFRAME;
extern const ShaderReplacementRule GRIDCUBE_CONSTANT_PICK;
extern const ShaderReplacementRule GRIDCUBE_CULLPOS_FROM_CENTER;
extern const ShaderReplacementRule GRIDCUBE_PLANE_CULLPOS_FROM_CENTER;

// Attribute-based rules for sparse volume grid quantities
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR;
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR;
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR;
extern const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR;


} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

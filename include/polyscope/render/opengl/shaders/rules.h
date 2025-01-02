// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/engine.h"

// clang-format off

namespace polyscope {
namespace render {
namespace backend_openGL3 {

extern const ShaderReplacementRule GLSL_VERSION;
extern const ShaderReplacementRule GLOBAL_FRAGMENT_FILTER;
extern const ShaderReplacementRule LIGHT_MATCAP;
extern const ShaderReplacementRule LIGHT_PASSTHRU;


// Shading color generation policies (colormapping, etc)
// all of these generate or modify `vec3 albedoColor` from some other data
extern const ShaderReplacementRule SHADE_BASECOLOR;             // constant from u_baseColor
extern const ShaderReplacementRule SHADE_COLOR;                 // from shadeColor
extern const ShaderReplacementRule SHADECOLOR_FROM_UNIFORM;             
extern const ShaderReplacementRule SHADE_COLORMAP_VALUE;        // colormapped from shadeValue
extern const ShaderReplacementRule SHADE_CATEGORICAL_COLORMAP;  // use ints to sample distinct values from colormap
extern const ShaderReplacementRule SHADE_COLORMAP_ANGULAR2;     // colormapped from angle of shadeValue2
extern const ShaderReplacementRule SHADE_GRID_VALUE2;           // generate a two-color grid with lines from shadeValue2
extern const ShaderReplacementRule SHADE_CHECKER_VALUE2;        // generate a two-color checker from shadeValue2
extern const ShaderReplacementRule SHADE_CHECKER_CATEGORY;      // generate a checker with colors from a categorical int
extern const ShaderReplacementRule SHADEVALUE_MAG_VALUE2;       // generate a shadeValue from the magnitude of shadeValue2
extern const ShaderReplacementRule ISOLINE_STRIPE_VALUECOLOR;   // modulate albedoColor based on shadeValue
extern const ShaderReplacementRule CONTOUR_VALUECOLOR;          // modulate albedoColor based on shadeValue
extern const ShaderReplacementRule CHECKER_VALUE2COLOR;         // modulate albedoColor based on shadeValue2
extern const ShaderReplacementRule SHADE_BASECOLOR;             // constant from u_baseColor
extern const ShaderReplacementRule PREMULTIPLY_COLOR;

// Positions, culling, etc
extern const ShaderReplacementRule GENERATE_VIEW_POS;          // computes viewPos, position in viewspace for fragment
extern const ShaderReplacementRule PROJ_AND_INV_PROJ_MAT;
extern const ShaderReplacementRule COMPUTE_SHADE_NORMAL_FROM_POSITION;
extern const ShaderReplacementRule PREMULTIPLY_LIT_COLOR;
extern const ShaderReplacementRule CULL_POS_FROM_VIEW;

ShaderReplacementRule generateSlicePlaneRule(std::string uniquePostfix);
ShaderReplacementRule generateVolumeGridSlicePlaneRule(std::string uniquePostfix);

// clang-format on

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

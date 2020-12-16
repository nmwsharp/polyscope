#pragma once

#include "polyscope/render/engine.h"

// clang-format off

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

extern const ShaderReplacementRule GLSL_VERSION;
extern const ShaderReplacementRule GLOBAL_FRAGMENT_FILTER;
extern const ShaderReplacementRule LIGHT_MATCAP;
extern const ShaderReplacementRule LIGHT_PASSTHRU;


// Shading color generation policies (colormapping, etc)
// all of these generate or modify `vec3 albedoColor` from some other data
extern const ShaderReplacementRule SHADE_BASECOLOR;             // constant from u_baseColor
extern const ShaderReplacementRule SHADE_COLOR;                 // from shadeColor
extern const ShaderReplacementRule SHADE_COLORMAP_VALUE;        // colormapped from shadeValue
extern const ShaderReplacementRule SHADE_COLORMAP_ANGULAR2;     // colormapped from angle of shadeValue2
extern const ShaderReplacementRule SHADE_GRID_VALUE2;           // generate a two-color grid with lines from shadeValue2
extern const ShaderReplacementRule SHADE_CHECKER_VALUE2;        // generate a two-color checker from shadeValue2
extern const ShaderReplacementRule SHADEVALUE_MAG_VALUE2;       // generate a shadeValue from the magnitude of shadeValue2
extern const ShaderReplacementRule ISOLINE_STRIPE_VALUECOLOR;   // modulate albedoColor based on shadeValue
extern const ShaderReplacementRule CHECKER_VALUE2COLOR;         // modulate albedoColor based on shadeValue2

// clang-format on

}
} // namespace render
} // namespace polyscope

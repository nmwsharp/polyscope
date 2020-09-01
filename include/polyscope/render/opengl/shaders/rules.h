#pragma once

#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

extern const ShaderReplacementRule GLSL_VERSION;
extern const ShaderReplacementRule GLOBAL_FRAGMENT_FILTER;
extern const ShaderReplacementRule LIGHT_MATCAP;
extern const ShaderReplacementRule LIGHT_PASSTHRU;


// Shading color generation policies (colormapping, etc)
extern const ShaderReplacementRule SHADE_BASECOLOR;
extern const ShaderReplacementRule SHADE_COLOR;
extern const ShaderReplacementRule SHADE_COLORMAP_VALUE;




}
} // namespace render
} // namespace polyscope

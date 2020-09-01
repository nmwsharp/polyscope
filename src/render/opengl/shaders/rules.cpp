#include "polyscope/render/opengl/shaders/rules.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

// possibly discards a fragment due to global rules
const ShaderReplacementRule GLSL_VERSION(
    /* rule name */ "GLSL_VERSION",
    /* replacement sources */
    {
        {"GLSL_VERSION", "#version 330 core"}, 
    }
);

// possibly discards a fragment due to global rules
const ShaderReplacementRule GLOBAL_FRAGMENT_FILTER(
    /* rule name */ "GLOBAL_FRAGMENT_FILTER",
    /* replacement sources */
    {
        {"GLOBAL_FRAGMENT_FILTER", "// do nothing, for now"}, 
    }
);


// light using a matcap texture
// input: vec3 albedoColor;
// output: vec3 litColor after lighting
const ShaderReplacementRule LIGHT_MATCAP (
    /* rule name */ "LIGHT_MATCAP",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler2D t_mat_r;
          uniform sampler2D t_mat_g;
          uniform sampler2D t_mat_b;
          uniform sampler2D t_mat_k;
          vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        )"},
      {"GENERATE_LIT_COLOR", "vec3 litColor = lightSurfaceMat(nHit, albedoColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k);"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {
      {"t_mat_r", 2},
      {"t_mat_g", 2},
      {"t_mat_b", 2},
      {"t_mat_k", 2},
    }
);

// "light" by just copying the value 
// input: vec3 albedoColor;
// output: vec3 litColor after lighting
const ShaderReplacementRule LIGHT_PASSTHRU (
    /* rule name */ "LIGHT_PASSTHRU",
    { /* replacement sources */
      {"GENERATE_LIT_COLOR", "vec3 litColor = albedoColor;"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


// input: uniform
// output: vec3 albedoColor
const ShaderReplacementRule SHADE_BASECOLOR (
    /* rule name */ "SHADE_BASECOLOR",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_baseColor;
        )"},
      {"GENERATE_SHADE_COLOR", "vec3 albedoColor = u_baseColor;"}
    },
    /* uniforms */ {
        {"u_baseColor", DataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

// input: vec3 shadeColor 
// output: vec3 albedoColor
const ShaderReplacementRule SHADE_COLOR(
    /* rule name */ "SHADE_COLOR",
    { /* replacement sources */
      {"GENERATE_SHADE_COLOR", "vec3 albedoColor = shadeColor;"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

// input: attribute float shadeValue
// output: vec3 albedoColor
const ShaderReplacementRule SHADE_COLORMAP_VALUE(
    /* rule name */ "SHADE_COLORMAP_VALUE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_rangeHigh;
          uniform float u_rangeLow;
          uniform sampler1D t_colormap;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
          float rangeTVal = (shadeValue - u_rangeLow) / (u_rangeHigh - u_rangeLow);
          rangeTVal = clamp(rangeTVal, 0.f, 1.f);
          vec3 albedoColor = texture(t_colormap, rangeTVal).rgb;
      )"}
    },
    /* uniforms */ {
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    },
    /* attributes */ {},
    /* textures */ {
        {"t_colormap", 1}
    }
);


// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

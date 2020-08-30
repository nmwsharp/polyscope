#include "polyscope/render/rules.h"

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


// shade a using a matcap texture
// input: vec3 albedoColor;
// output: vec3 litColor after lighting
const ShaderReplacementRule MATCAP_SHADE (
    /* rule name */ "MATCAP_SHADE",
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


const ShaderReplacementRule CYLINDER_PROPAGATE_VALUE (
    /* rule name */ "CYLINDER_PROPAGATE_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value;
          out float a_valueToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueToGeom = a_value;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_valueToGeom[];
          out float a_valueToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_valueToFrag = a_valueToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value", DataType::Float},
    },
    /* textures */ {}
);

// like propagate value, but takes two values at tip and taail and linearly interpolates
const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_VALUE (
    /* rule name */ "CYLINDER_PROPAGATE_BLEND_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value_tail;
          in float a_value_tip;
          out float a_valueTailToGeom;
          out float a_valueTipToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueTailToGeom = a_value_tail;
          a_valueTipToGeom = a_value_tip;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_valueTailToGeom[];
          in float a_valueTipToGeom[];
          out float a_valueTailToFrag;
          out float a_valueTipToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_valueTailToFrag = a_valueTailToGeom[0]; 
          a_valueTipToFrag = a_valueTipToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueTailToFrag;
          in float a_valueTipToFrag;
          float length2(vec3 x);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
          float shadeValue = mix(a_valueTailToFrag, a_valueTipToFrag, tEdge);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value_tail", DataType::Float},
      {"a_value_tip", DataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule CYLINDER_PROPAGATE_COLOR (
    /* rule name */ "CYLINDER_PROPAGATE_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color;
          out vec3 a_colorToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorToGeom = a_color;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorToGeom[];
          out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorToFrag = a_colorToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_colorToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec3 shadeColor = a_colorToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color", DataType::Vector3Float},
    },
    /* textures */ {}
);

// like propagate color, but takes two values at tip and taail and linearly interpolates
const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_COLOR (
    /* rule name */ "CYLINDER_PROPAGATE_BLEND_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color_tail;
          in vec3 a_color_tip;
          out vec3 a_colorTailToGeom;
          out vec3 a_colorTipToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorTailToGeom = a_color_tail;
          a_colorTipToGeom = a_color_tip;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorTailToGeom[];
          in vec3 a_colorTipToGeom[];
          out vec3 a_colorTailToFrag;
          out vec3 a_colorTipToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorTailToFrag = a_colorTailToGeom[0]; 
          a_colorTipToFrag = a_colorTipToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_colorTailToFrag;
          in vec3 a_colorTipToFrag;
          float length2(vec3 x);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
          vec3 shadeColor = mix(a_colorTailToFrag, a_colorTipToFrag, tEdge);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color_tail", DataType::Vector3Float},
      {"a_color_tip", DataType::Vector3Float},
    },
    /* textures */ {}
);


// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

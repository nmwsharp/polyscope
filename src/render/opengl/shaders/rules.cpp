// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/render/opengl/shaders/rules.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

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
      {"GENERATE_LIT_COLOR", R"(
          vec3 litColor = lightSurfaceMat(shadeNormal, albedoColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k);
      )"}
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
      {"u_baseColor", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

// input: uniform output: vec3 shadeColor
const ShaderReplacementRule SHADECOLOR_FROM_UNIFORM (
    /* rule name */ "SHADECOLOR_FROM_UNIFORM",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_color;
        )"},
      {"GENERATE_SHADE_COLOR", "vec3 shadeColor = u_color;"}
    },
    /* uniforms */ {
      {"u_color", RenderDataType::Vector3Float},
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
        {"u_rangeLow", RenderDataType::Float},
        {"u_rangeHigh", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {
        {"t_colormap", 1}
    }
);

const ShaderReplacementRule SHADE_CATEGORICAL_COLORMAP(
    /* rule name */ "SHADE_CATEGORICAL_COLORMAP",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler1D t_colormap;
          float intToDistinctReal(float start, int index);
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        // sample the categorical color
        int shadeInt = int(round(shadeValue));
        float startOffset = 0.;
        if(shadeInt < 0) { // do something sane for negative values
          shadeInt = -shadeInt;
          startOffset = 1./3.; // arbitrary value to shift the negative ones a bit
        }
        float catVal = intToDistinctReal(startOffset, shadeInt);
        vec3 albedoColor = texture(t_colormap, catVal).rgb;
      )"}
    },
    /* uniforms */ {
    },
    /* attributes */ {},
    /* textures */ {
        {"t_colormap", 1}
    }
);

// input: attribute vec2 shadeValue2
// output: vec3 albedoColor
const ShaderReplacementRule SHADE_COLORMAP_ANGULAR2(
    /* rule name */ "SHADE_COLORMAP_ANGULAR2",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_angle;
          uniform sampler1D t_colormap;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
          float pi = 3.14159265359;
          float angle = atan(shadeValue2.y, shadeValue2.x) / (2. * pi) + 0.5; // in [0,1]
          float shiftedAngle = mod(angle + u_angle/(2. * pi), 1.);
          vec3 albedoColor = texture(t_colormap, shiftedAngle).rgb;
      )"}
    },
    /* uniforms */ {
        {"u_angle", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {
        {"t_colormap", 1}
    }
);

const ShaderReplacementRule SHADE_GRID_VALUE2 (
    /* rule name */ "SHADE_GRID_VALUE2",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform vec3 u_gridLineColor;
          uniform vec3 u_gridBackgroundColor;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        float mX = mod(shadeValue2.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(shadeValue2.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min(min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float width = 0.05;
        float slopeWidthPix = 5.;
        vec2 fw = fwidth(shadeValue2);
        float scale = max(fw.x, fw.y);
        float pWidth = slopeWidthPix * scale;
        float s = smoothstep(width, width + pWidth, minD);
        vec3 albedoColor = mix(u_gridLineColor, u_gridBackgroundColor, s);
      )"}
    },
    /* uniforms */ {
       {"u_modLen", RenderDataType::Float},
       {"u_gridLineColor", RenderDataType::Vector3Float},
       {"u_gridBackgroundColor", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule SHADE_CHECKER_VALUE2 (
    /* rule name */ "SHADE_CHECKER_VALUE2",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform vec3 u_color1;
          uniform vec3 u_color2;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        // NOTE checker math shared with other shaders
        float mX = mod(shadeValue2.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(shadeValue2.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        // TODO do some clever screen space derivative thing to prevent aliasing
        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);
        vec3 albedoColor = mix(u_color1, u_color2, s);
      )"}
    },
    /* uniforms */ {
       {"u_modLen", RenderDataType::Float},
       {"u_color1", RenderDataType::Vector3Float},
       {"u_color2", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule SHADE_CHECKER_CATEGORY(
    /* rule name */ "SHADE_CHECKER_CATEGORY",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform float u_modDarkness;
          uniform sampler1D t_colormap;

          float intToDistinctReal(float start, int index);
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        // sample the categorical color
        float catVal = intToDistinctReal(0., int(shadeValue));
        float scaleFac = 1.2f; // pump up the brightness a bit, so the modDarkness doesn't make it too dark
        vec3 catColor = scaleFac * texture(t_colormap, catVal).rgb;
        vec3 catColorDark = catColor * u_modDarkness;

        // NOTE checker math shared with other shaders
        float mX = mod(shadeValue2.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(shadeValue2.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        // TODO do some clever screen space derivative thing to prevent aliasing
        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);
        vec3 albedoColor = mix(catColor, catColorDark, s);
      )"}
    },
    /* uniforms */ {
       {"u_modLen", RenderDataType::Float},
       {"u_modDarkness", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {
        {"t_colormap", 1}
    }
);

// input vec2 shadeValue2
// output: float shadeValue
const ShaderReplacementRule SHADEVALUE_MAG_VALUE2(
    /* rule name */ "SHADEVALUE_MAG_VALUE2",
    { /* replacement sources */
      {"GENERATE_SHADE_COLOR", "float shadeValue = length(shadeValue2);"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


const ShaderReplacementRule ISOLINE_STRIPE_VALUECOLOR (
    /* rule name */ "ISOLINE_STRIPE_VALUECOLOR",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform float u_modDarkness;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        float modVal = mod(shadeValue, 2.0 * u_modLen);
        if(modVal > u_modLen) {
          albedoColor *= u_modDarkness;
        }
      )"}
    },
    /* uniforms */ {
        {"u_modLen", RenderDataType::Float},
        {"u_modDarkness", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);


const ShaderReplacementRule CONTOUR_VALUECOLOR (
    /* rule name */ "CONTOUR_VALUECOLOR",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform float u_modThickness;
          uniform float u_modDarkness;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        /* TODO: get rid of arbitrary constants */
        vec2 gradF = vec2( dFdx(shadeValue), dFdy(shadeValue) );
        float w = 1./( 10. / u_modLen * u_modThickness * length(gradF) );
        float s = u_modDarkness * exp( -pow( w*(fract(abs(shadeValue/u_modLen))-0.5), 8.0 ));
        albedoColor *= 1.-s;
      )"}
    },
    /* uniforms */ {
        {"u_modLen", RenderDataType::Float},
        {"u_modThickness", RenderDataType::Float},
        {"u_modDarkness", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule CHECKER_VALUE2COLOR (
    /* rule name */ "CHECKER_VALUE2COLOR",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_modLen;
          uniform float u_modDarkness;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        vec3 albedoColorDark = albedoColor * u_modDarkness;
        float mX = mod(shadeValue2.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(shadeValue2.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);
        albedoColor = mix(albedoColor, albedoColorDark, s);
      )"}
    },
    /* uniforms */ {
       {"u_modLen", RenderDataType::Float},
       {"u_modDarkness", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);


const ShaderReplacementRule GENERATE_VIEW_POS (
    /* rule name */ "GENERATE_VIEW_POS",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform mat4 u_invProjMatrix_viewPos; // weird names are to unique-ify because we have multiple....
          uniform vec4 u_viewport_viewPos;
          vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        )"},
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
        vec2 depthRange_viewPos = vec2(gl_DepthRange.near, gl_DepthRange.far);
        vec4 fragCoord_viewPos = gl_FragCoord;
        fragCoord_viewPos.z = depth;
        vec3 viewPos = fragmentViewPosition(u_viewport_viewPos, depthRange_viewPos, u_invProjMatrix_viewPos, fragCoord_viewPos);
      )"}
    },
    /* uniforms */ {
      {"u_invProjMatrix_viewPos", RenderDataType::Matrix44Float},
      {"u_viewport_viewPos", RenderDataType::Vector4Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule PROJ_AND_INV_PROJ_MAT (
    /* rule name */ "PROJ_AND_INV_PROJ_MAT",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        )"}
    },
    /* uniforms */ {
        {"u_invProjMatrix", RenderDataType::Matrix44Float},
        {"u_viewport", RenderDataType::Vector4Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule COMPUTE_SHADE_NORMAL_FROM_POSITION (
    /* rule name */ "COMPUTE_SHADE_NORMAL_FROM_POSITION",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
        vec2 depthRange_fornormal = vec2(gl_DepthRange.near, gl_DepthRange.far);
        vec3 viewPos_fornormal = fragmentViewPosition(u_viewport, depthRange_fornormal, u_invProjMatrix, gl_FragCoord);
        shadeNormal = normalize(cross(dFdx(viewPos_fornormal),dFdy(viewPos_fornormal)));
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule PREMULTIPLY_LIT_COLOR(
    /* rule name */ "PREMULTIPLY_LIT_COLOR",
    { /* replacement sources */
      {"PERTURB_LIT_COLOR", R"(
        litColor *= alphaOut; // premultiplied alpha
      )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


// TODO delete me
const ShaderReplacementRule CULL_POS_FROM_VIEW (
    /* rule name */ "CULL_POS_FROM_VIEW",
    { /* replacement sources */
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
        vec3 cullPos = viewPos;
      )"},
    },
    /* uniforms */ {
    },
    /* attributes */ {},
    /* textures */ {}
);


ShaderReplacementRule generateSlicePlaneRule(std::string uniquePostfix) {

  std::string centerUniformName = "u_slicePlaneCenter_" + uniquePostfix;
  std::string normalUniformName = "u_slicePlaneNormal_" + uniquePostfix;

  // This takes what is otherwise a simple rule, and substitues uniquely named uniforms so that we can have multiple slice planes
  ShaderReplacementRule slicePlaneRule (
      /* rule name */ "SLICE_PLANE_CULL_" + uniquePostfix,
      { /* replacement sources */
        {"FRAG_DECLARATIONS", "uniform vec3 " + centerUniformName + "; uniform vec3 " + normalUniformName + ";"},
        {"GLOBAL_FRAGMENT_FILTER", 
         "if(dot(cullPos, " + normalUniformName + ") < dot( " + centerUniformName + " , " + normalUniformName + ")) { discard; }"}
      },
      /* uniforms */ {
        {centerUniformName, RenderDataType::Vector3Float},
        {normalUniformName, RenderDataType::Vector3Float},
      },
      /* attributes */ {},
      /* textures */ {}
  );

  return slicePlaneRule;
}

ShaderReplacementRule generateVolumeGridSlicePlaneRule(std::string uniquePostfix) {

  std::string centerUniformName = "u_slicePlaneCenter_" + uniquePostfix;
  std::string normalUniformName = "u_slicePlaneNormal_" + uniquePostfix;

  // This takes what is otherwise a simple rule, and substitues uniquely named uniforms so that we can have multiple slice planes
  ShaderReplacementRule slicePlaneRule (
      /* rule name */ "SLICE_PLANE_VOLUMEGRID_CULL_" + uniquePostfix,
      { /* replacement sources */
        // skip the frag declarations, we will already have them from the other rule
        // {"FRAG_DECLARATIONS", "uniform vec3 " + centerUniformName + "; uniform vec3 " + normalUniformName + ";"},
        {"GRID_PLANE_NEIGHBOR_FILTER",
         "if(dot(neighCullPos, " + normalUniformName + ") < dot( " + centerUniformName + " , " + normalUniformName + ")) { neighIsVisible = false; }"
        }
      },
      /* uniforms */ {
        {centerUniformName, RenderDataType::Vector3Float},
        {normalUniformName, RenderDataType::Vector3Float},
      },
      /* attributes */ {},
      /* textures */ {}
  );

  return slicePlaneRule;
}

// clang-format on

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

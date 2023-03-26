// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/lighting_shaders.h"
#include "polyscope/render/opengl/shaders/texture_draw_shaders.h"

// clang-format off

namespace polyscope {
namespace render{
namespace backend_openGL3_glfw {

const ShaderStageSpecification MAP_LIGHT_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_exposure", RenderDataType::Float},
        {"u_gamma", RenderDataType::Float},
        {"u_whiteLevel", RenderDataType::Float},
        {"u_texelSize", RenderDataType::Vector2Float},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_image", 2},
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform float u_exposure;
      uniform float u_whiteLevel;
      uniform float u_gamma;
      uniform vec2 u_texelSize;
      layout (location = 0) out vec4 outputVal;

      //vec2 texelSize = vec2(1.0 / textureWidth, 1.0 / textureHeight);

      vec4 sampleSingle(vec2 tCoord) {
          vec4 sampleVal = texture(t_image, tCoord);

          ${ SAMPLE_SINGLE }$

          return sampleVal;
      }

      vec4 imageSample() {
  
        // This function is written like this to hopefully make it as easy as possible to unroll

        vec4 result = vec4(0., 0., 0., 0.);

        ${ DOWNSAMPLE_RESOLVE }$
          
        return result / (downsampleFactor * downsampleFactor);
      } 

      void main() {

        vec4 color4 = imageSample();
        vec3 color = color4.rgb;
        float alpha = color4.a;

        // "lighting"
        color = color * u_exposure;

        // tonemapping (extended Reinhard)
        vec3 num = color * (1.0f + (color / vec3(u_whiteLevel * u_whiteLevel)));
        vec3 den = (1.0f + color);
        color = num / den;
        
        // gamma correction
        color = pow(color, vec3(1.0/u_gamma));  
       
        outputVal = vec4(color, alpha);
    }  
)"
};

// === Rules

const ShaderReplacementRule DOWNSAMPLE_RESOLVE_1 (
    /* rule name */ "DOWNSAMPLE_RESOLVE_1",
    { /* replacement sources */
      {"DOWNSAMPLE_RESOLVE", R"(
          result += sampleSingle(tCoord);
          result.x += 1e-8*u_texelSize.x; // prevent u_texelSize from being optimized out
          int downsampleFactor = 1;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule DOWNSAMPLE_RESOLVE_2 (
    /* rule name */ "DOWNSAMPLE_RESOLVE_2",
    { /* replacement sources */
      {"DOWNSAMPLE_RESOLVE", R"(
          float fac = 0.5;
          vec2 tCoordStart = tCoord - vec2(-fac, -fac)*u_texelSize;
          for(int i = 0; i < 2; i++) {
            for(int j = 0; j < 2; j++) {
              result += sampleSingle(tCoordStart + vec2(i,j) * u_texelSize);
            }
          }
          int downsampleFactor = 2;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule DOWNSAMPLE_RESOLVE_3 (
    /* rule name */ "DOWNSAMPLE_RESOLVE_3",
    { /* replacement sources */
      {"DOWNSAMPLE_RESOLVE", R"(
          float fac = 1.;
          vec2 tCoordStart = tCoord - vec2(-fac, -fac)*u_texelSize;
          for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
              result += sampleSingle(tCoordStart + vec2(i,j) * u_texelSize);
            }
          }
          int downsampleFactor = 3;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule DOWNSAMPLE_RESOLVE_4 (
    /* rule name */ "DOWNSAMPLE_RESOLVE_4",
    { /* replacement sources */
      {"DOWNSAMPLE_RESOLVE", R"(
          float fac = 1.5;
          vec2 tCoordStart = tCoord - vec2(-fac, -fac)*u_texelSize;
          for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
              result += sampleSingle(tCoordStart + vec2(i,j) * u_texelSize);
            }
          }
          int downsampleFactor = 4;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


const ShaderReplacementRule TRANSPARENCY_RESOLVE_SIMPLE (
    /* rule name */ "TRANSPARENCY_RESOLVE_SIMPLE ",
    { /* replacement sources */
      {"SAMPLE_SINGLE", R"(
      sampleVal.xyz = sampleVal.xyz / (sampleVal.w + 1e-4);
      //sampleVal.w = clamp(sampleVal.w - .5, 0., 1.); // subtract .5 corresponds to default background transparency
      sampleVal.w = clamp(sampleVal.w, 0., 1.);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule TRANSPARENCY_STRUCTURE (
    /* rule name */ "TRANSPARENCY_STRUCTURE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_transparency;
        )"},
      {"GENERATE_ALPHA", R"(
          alphaOut = u_transparency;
        )"},
    },
    /* uniforms */ {
        {"u_transparency", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule TRANSPARENCY_PEEL_STRUCTURE (
    /* rule name */ "TRANSPARENCY_PEEL_STRUCTURE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_transparency;
          uniform sampler2D t_minDepth;
          uniform vec2 u_viewportDim;
        )"},
      {"GENERATE_ALPHA", R"(
          alphaOut = u_transparency;
        )"},
      {"GLOBAL_FRAGMENT_FILTER", R"(
          // assumption: "float depth" must be already set 
          // (use float depth = gl_FragCoord.z; if not doing anything special)
          vec2 depthPixelCoords = gl_FragCoord.xy / u_viewportDim;
          float minDepth = texture(t_minDepth, depthPixelCoords).x;
          if(depth <= minDepth+1e-6) {
            discard;
          }
        )"},
    },
    /* uniforms */ {
        {"u_transparency", RenderDataType::Float},
        {"u_viewportDim", RenderDataType::Vector2Float},
    },
    /* attributes */ {},
    /* textures */ {
        {"t_minDepth", 2},
    }
);

const ShaderReplacementRule TRANSPARENCY_PEEL_GROUND (
    /* rule name */ "TRANSPARENCY_PEEL_GROUND",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler2D t_minDepth;
        )"},
      {"GLOBAL_FRAGMENT_FILTER", R"(
          // assumption: "float depth" must be already set 
          // (use float depth = gl_FragCoord.z; if not doing anything special)
          vec2 depthPixelCoords = gl_FragCoord.xy / u_viewportDim;
          float minDepth = texture(t_minDepth, depthPixelCoords).x;
          if(depth <= minDepth+1e-6) {
            discard;
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {
        {"t_minDepth", 2},
    }
);

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

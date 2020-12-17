// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

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
        {"u_exposure", DataType::Float},
        {"u_gamma", DataType::Float},
        {"u_whiteLevel", DataType::Float},
        {"u_texelSize", DataType::Vector2Float},
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
          result += texture(t_image, tCoord);
          result.x += 0.*u_texelSize.x; // prevent u_texelSize from being optimized out
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
              result += texture(t_image, tCoordStart + vec2(i,j) * u_texelSize);
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
              result += texture(t_image, tCoordStart + vec2(i,j) * u_texelSize);
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
              result += texture(t_image, tCoordStart + vec2(i,j) * u_texelSize);
            }
          }
          int downsampleFactor = 4;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

// clang-format on

}
} // namespace render
} // namespace polyscope

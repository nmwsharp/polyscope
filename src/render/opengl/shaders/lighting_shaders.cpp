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

      vec4 imageSample(vec2 coord) {
  
        // This function is written like this to hopefully make it as easy as possible to unroll

        vec4 result = vec4(0., 0., 0., 0.);

        ${ DOWNSAMPLE_RESOLVE }$
          
        return result / (downsampleFactor * downsampleFactor);
      } 
        
      ${ AA_DECL }$

      void main() {

        ${ AA_APPLY }$

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
          result += texture(t_image, coord);
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
          vec2 tCoordStart = coord - vec2(-fac, -fac)*u_texelSize;
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
          vec2 tCoordStart = coord - vec2(-fac, -fac)*u_texelSize;
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
          vec2 tCoordStart = coord - vec2(-fac, -fac)*u_texelSize;
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

const ShaderReplacementRule NOAA (
    /* rule name */ "NOAA",
    { /* replacement sources */
      {"AA_APPLY", R"(
        vec4 color4 = imageSample(tCoord);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule FXAA (
// adapted from https://github.com/mattdesl/glsl-fxaa under MIT license
    /* rule name */ "FXAA",
    { /* replacement sources */
      {"AA_DECL", R"(
              
#ifndef FXAA_REDUCE_MIN
  #define FXAA_REDUCE_MIN   (1.0/ 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
  #define FXAA_REDUCE_MUL   (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
  #define FXAA_SPAN_MAX     8.0
#endif

        void texcoords(vec2 fragCoord,
              out vec2 v_rgbNW, out vec2 v_rgbNE,
              out vec2 v_rgbSW, out vec2 v_rgbSE,
              out vec2 v_rgbM) {
          v_rgbNW = (fragCoord + vec2(-1.0, -1.0)) * u_texelSize ;
          v_rgbNE = (fragCoord + vec2(1.0, -1.0)) * u_texelSize ;
          v_rgbSW = (fragCoord + vec2(-1.0, 1.0)) * u_texelSize ;
          v_rgbSE = (fragCoord + vec2(1.0, 1.0)) * u_texelSize ;
          v_rgbM = vec2(fragCoord * u_texelSize );
        }

        vec4 fxaa(vec2 fragCoord, vec2 v_rgbNW, vec2 v_rgbNE, 
                    vec2 v_rgbSW, vec2 v_rgbSE, 
                    vec2 v_rgbM) {
            vec4 color;
            vec3 rgbNW = imageSample(v_rgbNW).xyz;
            vec3 rgbNE = imageSample(v_rgbNE).xyz;
            vec3 rgbSW = imageSample(v_rgbSW).xyz;
            vec3 rgbSE = imageSample(v_rgbSE).xyz;
            vec4 texColor = imageSample(v_rgbM);
            vec3 rgbM  = texColor.xyz;
            vec3 luma = vec3(0.299, 0.587, 0.114);
            float lumaNW = dot(rgbNW, luma);
            float lumaNE = dot(rgbNE, luma);
            float lumaSW = dot(rgbSW, luma);
            float lumaSE = dot(rgbSE, luma);
            float lumaM  = dot(rgbM,  luma);
            float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
            float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
            
            vec2 dir;
            dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
            dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
            
            float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                                  (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
            
            float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
            dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
                      max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                      dir * rcpDirMin)) * u_texelSize ;
            
            vec3 rgbA = 0.5 * (
                imageSample(fragCoord * u_texelSize  + dir * (1.0 / 3.0 - 0.5)).xyz +
                imageSample(fragCoord * u_texelSize  + dir * (2.0 / 3.0 - 0.5)).xyz);
            vec3 rgbB = rgbA * 0.5 + 0.25 * (
                imageSample(fragCoord * u_texelSize  + dir * -0.5).xyz +
                imageSample(fragCoord * u_texelSize  + dir * 0.5).xyz);

            float lumaB = dot(rgbB, luma);
            if ((lumaB < lumaMin) || (lumaB > lumaMax))
                color = vec4(rgbA, texColor.a);
            else
                color = vec4(rgbB, texColor.a);
            return color;
        }

        vec4 apply(vec2 fragCoord) {
          vec2 v_rgbNW;
          vec2 v_rgbNE;
          vec2 v_rgbSW;
          vec2 v_rgbSE;
          vec2 v_rgbM;

          //compute the texture coords
          texcoords(fragCoord, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
          
          //compute FXAA
          return fxaa(fragCoord, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
        }
        )"},
      {"AA_APPLY", R"(
      vec4 color4 = apply(tCoord / u_texelSize);
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

// clang-format on

}
} // namespace render
} // namespace polyscope

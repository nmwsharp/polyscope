// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/histogram_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

// clang-format off

const ShaderStageSpecification HISTOGRAM_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    {}, // uniforms

    // attributes
    {
        {"a_coord", RenderDataType::Vector2Float},
    },

    {}, // textures

    // source
R"(
      ${ GLSL_VERSION }$
      in vec2 a_coord;
      
      out float shadeValueRaw;

      void main()
      {
          shadeValueRaw = a_coord.x;
          vec2 scaledCoord = vec2(a_coord.x, a_coord.y * .85);
          gl_Position = vec4(2.*scaledCoord - vec2(1.0, 1.0),0.,1.);
      }
)"
};

const ShaderStageSpecification HISTOGRAM_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in float shadeValueRaw;

      ${ FRAG_DECLARATIONS }$

      layout(location = 0) out vec4 outputF;

      void main()
      {

        float shadeValue = shadeValueRaw;

        ${ GENERATE_SHADE_COLOR }$

        // Darken when outside range
        float darkFactor = 1.0;
        if(shadeValue < u_rangeLow || shadeValue > u_rangeHigh) {
          darkFactor = 0.6;
        }

        outputF = vec4(darkFactor*albedoColor.rgb, 1.0);
      }
)"
};

const ShaderStageSpecification HISTOGRAM_CATEGORICAL_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
      {"u_dataRangeLow", RenderDataType::Float},
      {"u_dataRangeHigh", RenderDataType::Float}
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in float shadeValueRaw;
      uniform float u_dataRangeLow;
      uniform float u_dataRangeHigh;

      ${ FRAG_DECLARATIONS }$

      layout(location = 0) out vec4 outputF;

      void main()
      {

        // Used to restore [0,1] tvals to the orininal data range for the categorical int remapping
        float shadeValue = mix(u_dataRangeLow, u_dataRangeHigh, shadeValueRaw);

        ${ GENERATE_SHADE_COLOR }$

        outputF = vec4(albedoColor.rgb, 1.0);
      }
)"
};

// clang-format on

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

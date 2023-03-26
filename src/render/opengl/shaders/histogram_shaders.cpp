// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/histogram_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

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
      
      out float t;

      void main()
      {
          t = a_coord.x;
          vec2 scaledCoord = vec2(a_coord.x, a_coord.y * .85);
          gl_Position = vec4(2.*scaledCoord - vec2(1.0, 1.0),0.,1.);
      }
)"
};

const ShaderStageSpecification HISTOGRAM_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
      {"u_cmapRangeMin", RenderDataType::Float},
      {"u_cmapRangeMax", RenderDataType::Float}
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_colormap", 1}
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in float t;

      uniform sampler1D t_colormap;
      uniform float u_cmapRangeMin;
      uniform float u_cmapRangeMax;

      layout(location = 0) out vec4 outputF;

      void main()
      {
        float mapT = (t - u_cmapRangeMin) / (u_cmapRangeMax - u_cmapRangeMin); 
        float clampMapT = clamp(mapT, 0.f, 1.f);

        // Darken when outside range
        float darkFactor = 1.0;
        if(clampMapT != mapT) {
          darkFactor = 0.6;
        }

        outputF = vec4(darkFactor*texture(t_colormap, clampMapT).rgb, 1.0);
      }
)"
};

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

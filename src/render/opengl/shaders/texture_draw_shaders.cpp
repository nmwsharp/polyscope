// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

// clang-format off

namespace polyscope {
namespace render{

const ShaderStageSpecification  TEXTURE_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    { },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
    },

    // textures
    {},
    
    // source
    POLYSCOPE_GLSL(150,
      in vec3 a_position;
      out vec2 tCoord;

      void main()
      {
          tCoord = (a_position.xy+vec2(1.0,1.0))/2.0;
          gl_Position = vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification PLAIN_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      in vec2 tCoord;
      uniform sampler2D t_image;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        outputF = vec4(texture(t_image, tCoord).rgba);
      }
    )
};

const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_mapDot", DataType::Vector3Float},
    }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform vec3 u_mapDot;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        float sampleVal = dot(u_mapDot, texture(t_image, tCoord).rgb);
        outputF = vec4(sampleVal, 0., 0., 1.);
      }
    )
};

const ShaderStageSpecification MAP3_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_scale", DataType::Vector3Float},
        {"u_shift", DataType::Vector3Float},
    }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform vec3 u_scale;
      uniform vec3 u_shift;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        vec3 val = texture(t_image, tCoord).rgb;
        vec3 mapped = (val + u_shift) * u_scale;
        outputF = vec4(mapped, 1.);
      }
    )
};

// clang-format on

} // namespace render
} // namespace polyscope

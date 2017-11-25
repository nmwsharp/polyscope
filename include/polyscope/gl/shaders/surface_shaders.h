#pragma once

static const VertShader PLAIN_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      out vec3 Normal;
      out vec3 Position;

      void main()
      {
          Position = a_position;
          Normal = a_normal;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(Position,1.);
      }
    )
};

static const FragShader PLAIN_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_color", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(150,
      uniform vec3 u_eye;
      uniform vec3 u_lightCenter;
      uniform float u_lightDist;
      uniform vec3 u_color;
      in vec3 Normal;
      in vec3 Position;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );

      void main()
      {
        outputF = lightSurface(Position, Normal, u_color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};

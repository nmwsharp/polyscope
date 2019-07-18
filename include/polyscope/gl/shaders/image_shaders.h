// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

// clang-format off

namespace polyscope {
namespace gl {

static const VertShader PROJECTEDIMAGE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_tCoord", GLData::Vector2Float},
    },

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec2 a_tCoord;
      out vec2 tCoord;

      void main()
      {
          tCoord = a_tCoord;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

static const FragShader PROJECTEDIMAGE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_transparency", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_image", 2}
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform vec3 u_wirecolor;
      in vec2 tCoord;

      uniform float u_transparency;
      uniform sampler2D t_image;

      out vec4 outputF;


      void main()
      {
        outputF = vec4(texture(t_image, tCoord).rgb, u_transparency);
      }

    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

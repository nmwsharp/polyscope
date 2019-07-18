// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

// clang-format off

namespace polyscope {
namespace gl {

static const VertShader TEXTURE_DRAW_VERT_SHADER =  {
    
    // uniforms
    {
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
      in vec3 a_position;
      in vec2 a_tcoord;
      out vec2 tCoord;

      void main()
      {
          tCoord = (a_position.xy+vec2(1.0,1.0))/2.0 + .00001 * a_tcoord;
          gl_Position = vec4(a_position,1.);
      }
    )
};

static const FragShader TEXTURE_DRAW_FRAG_SHADER = {
    
    // uniforms
    {
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
      in vec2 tCoord;

      uniform sampler2D t_image;

      out vec4 outputF;


      void main()
      {
        outputF = vec4(texture(t_image, tCoord).rgba);
      }

    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/shaders.h"

// clang-format off

namespace polyscope {
namespace render{

const ShaderStageSpecification  TEXTURE_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    {
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
    },

    // textures
    {},
    
    // outputs
    "",

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

const ShaderStageSpecification TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
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

} // namespace render
} // namespace polyscope

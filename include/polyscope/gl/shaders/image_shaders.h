#pragma once

static const VertShader PROJECTEDIMAGE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_tCoord", GLData::Vector2Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec2 a_tCoord;
      out vec2 tCoord;

      void main()
      {
          tCoord = a_tCoord;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position,1.);
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
    GLSL(150,
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
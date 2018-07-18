#pragma once

static const VertShader TEXTURE_DRAW_VERT_SHADER =  {
    
    // uniforms
    {
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_tcoord", GLData::Vector2Float},
    },

    // source
    GLSL(150,
      in vec3 a_position;
      in vec2 a_tcoord;
      out vec2 tCoord;

      void main()
      {
          tCoord = a_tcoord;
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
    GLSL(150,
      in vec2 tCoord;

      uniform sampler2D t_image;

      out vec4 outputF;


      void main()
      {
        //outputF = vec4(texture(t_image, tCoord).rgb, 0.5);
        //outputF = vec4(texture(t_image, tCoord).rgb, 0.5);
        //outputF = 0.01 * vec4(texture(t_image, tCoord).rgba) + vec4(texture(t_image, vec2(0.3, 0.5)).rgba);
        //outputF = 0.5 * vec4(texture(t_image, tCoord).rgb, 0.5) + 0.5 * vec4(0.3f + tCoord.x, 0.2f + tCoord.y, 0.5f, 0.6f);
        outputF = 0.9 * vec4(texture(t_image, tCoord).rgb, 0.5) + 0.1 * vec4(0.3f + tCoord.x, 0.2f + tCoord.y, 0.5f, 0.6f);
        //outputF = vec4(tCoord.x, tCoord.y, 0.5f, 0.6f);
      }

    )
};

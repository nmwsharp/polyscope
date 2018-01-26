#pragma once

static const VertShader HISTOGRAM_VERT_SHADER =  {
    
    // uniforms
    {
    },

    // attributes
    {
        {"a_coord", GLData::Vector2Float},
    },

    // source
    GLSL(150,
      in vec2 a_coord;
      
      out float t;

      void main()
      {
          t = a_coord.x;
          vec2 scaledCoord = vec2(a_coord.x, a_coord.y * .85);
          gl_Position = vec4(2.*scaledCoord - vec2(1.0, 1.0),0.,1.);
      }
    )
};

static const FragShader HISTORGRAM_FRAG_SHADER = {
    
    // uniforms
    {
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_colormap", 1}
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(330,
      in float t;

      uniform sampler1D t_colormap;

      layout(location = 0) out vec4 outputF;


      void main()
      {
        outputF = vec4(texture(t_colormap, t).rgb, 1.0);
      }
    )
};

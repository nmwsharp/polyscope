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
      {"u_cmapRangeMin", GLData::Float},
      {"u_cmapRangeMax", GLData::Float}
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
      uniform float u_cmapRangeMin;
      uniform float u_cmapRangeMax;

      layout(location = 0) out vec4 outputF;


      void main()
      {
        float mapT = (t - u_cmapRangeMin) / (u_cmapRangeMax - u_cmapRangeMin); 
        mapT = clamp(mapT, 0.f, 1.f);
        outputF = vec4(texture(t_colormap, mapT).rgb, 1.0);
      }
    )
};

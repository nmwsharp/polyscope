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
      
      out vec2 coord;

      void main()
      {
          vec2 scaledCoord = vec2(a_coord.x, a_coord.y * .85);
          coord = scaledCoord;
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
        //{"t_image", 2}
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(330,
      in vec2 coord;

      //uniform sampler2D t_image;

      layout(location = 0) out vec4 outputF;


      void main()
      {
        //// Test if below curve
        ////float tSmooth = tRange;
        //float tSmooth = smoothstep(0.f, 1.f, tRange);
        //float curveValHere = mix(curveVals.x, curveVals.y, tRange);
        //if(coord.y > curveValHere) {
          //discard;
        //} 

        // If below curve, color in
        //outputF = vec4(texture(t_image, tCoord).rgb, u_transparency);
        outputF = vec4(0.5, 0.5, 0.5, 0.5);
      }
    )
};

#pragma once

static const VertShader WIREFRAME_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;

      void main()
      {
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

static const FragShader WIREFRAME_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_wirecolor", GLData::Vector3Float},
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
      uniform vec3 u_wirecolor;
      out vec4 outputF;


      void main()
      {
        outputF = vec4(u_wirecolor, 1.0);
      }

    )
};
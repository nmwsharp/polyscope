#pragma once

static const VertShader WIREFRAME_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_edgeDists", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_edgeDists;
      out vec3 Position;
      out vec3 EdgeDists;

      void main()
      {
          Position = a_position;
          EdgeDists = a_edgeDists;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(Position,1.);
      }
    )
};

static const FragShader WIREFRAME_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_wirecolor", GLData::Vector3Float},
        {"u_edgeWidth", GLData::Float},
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
      uniform float u_edgeWidth;
      uniform vec3 u_wirecolor;
      in vec3 Position;
      in vec3 EdgeDists;
      out vec4 outputF;


      void main()
      {

        float eDist = min(EdgeDists.x, min(EdgeDists.y, EdgeDists.z));
        float eFactor = 0.;
        if(eDist < u_edgeWidth) {
          eFactor = 1.0;
        }

        outputF = eFactor * vec4(u_wirecolor, 1.0) + (1.0 - eFactor) * vec4(1, 1, 1, 0.0);
      }

    )
};
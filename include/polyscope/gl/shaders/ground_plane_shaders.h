#pragma once

static const VertShader GROUND_PLANE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector4Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec4 a_position;
      out vec3 Normal;
      out vec4 PositionWorldHomog;

      void main()
      {
          Normal = mat3(u_viewMatrix) * vec3(0., 1., 0.);
          gl_Position = u_projMatrix * u_viewMatrix * a_position;
          PositionWorldHomog = a_position;
      }
    )
};

static const FragShader GROUND_PLANE_FRAG_SHADER = {
    
    // uniforms
    {
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_ground", 2},
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(150,

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_ground;
      in vec3 Normal;
      in vec4 PositionWorldHomog;
      out vec4 outputF;

      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      void main()
      {
        vec2 coordXZ = PositionWorldHomog.xz / PositionWorldHomog.w;
        coordXZ /= 10;
        vec4 color = texture(t_ground, coordXZ);
        //vec4 colorMod = vec4(mod(coordXZ.x, 1.0), mod(coordXZ.y, 1.0), 0.0, 1.0);
        outputF = lightSurfaceMat(Normal, color.xyz, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader SURFACE_WIREFRAME_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
        {"a_barycoord", GLData::Vector3Float},
        {"a_edgeReal", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in vec3 a_edgeReal;
      out vec3 Normal;
      out vec3 Barycoord;
      out vec3 EdgeReal;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          EdgeReal = a_edgeReal;
          Barycoord = a_barycoord;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

static const FragShader SURFACE_WIREFRAME_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_edgeColor", GLData::Vector3Float},
        {"u_edgeWidth", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform float u_edgeWidth;
      uniform vec3 u_edgeColor;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      in vec3 Normal;
      in vec3 Barycoord;
      in vec3 EdgeReal;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      float getEdgeFactor(vec3 UVW, vec3 edgeReal, float width) {
          // The Nick Sharp Edge Function (tm). There are many like it, but this one is mine.
          float slopeWidth = 1.;
          
          vec3 fw = fwidth(UVW);
          vec3 realUVW = max(UVW, 1.0 - edgeReal.yzx);
          vec3 baryWidth = slopeWidth * fw;

          vec3 end = width*fw;
          vec3 dist = smoothstep(end - baryWidth, end, realUVW);

          float e = 1.0 - min(min(dist.x, dist.y), dist.z);
          return e;
      }

      void main()
      {

        vec3 color = u_edgeColor;
        float alpha = getEdgeFactor(Barycoord, EdgeReal, u_edgeWidth);

        vec4 outputColor = lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b);
        outputColor.w = alpha;
        outputF = outputColor;
      }

    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

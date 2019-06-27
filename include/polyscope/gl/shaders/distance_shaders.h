#pragma once

static const VertShader VERT_DIST_SURFACE_VERT_SHADER =  {
    
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
        {"a_colorval", GLData::Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in float a_colorval;
      out vec3 Normal;
      out vec3 Barycoord;
      out float Colorval;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Barycoord = a_barycoord;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

static const FragShader VERT_DIST_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_basecolor", GLData::Vector3Float},
        {"u_edgeWidth", GLData::Float},
        {"u_rangeLow", GLData::Float},
        {"u_rangeHigh", GLData::Float},
        {"u_modLen", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_colormap", 1}
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(150,
      uniform float u_edgeWidth;
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform float u_modLen;
      uniform vec3 u_basecolor;
      uniform sampler1D t_colormap;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      in vec3 Normal;
      in vec3 Barycoord;
      in float Colorval;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
      float getEdgeFactor(vec3 UVW, float width);

      vec3 surfaceColor() {
        float t = (Colorval - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      vec3 edgeColor(vec3 surfaceColor) {
          vec3 edgeColor = vec3(0.0, 0.0, 0.0);
          float eFactor = getEdgeFactor(Barycoord, u_edgeWidth);
          return eFactor * edgeColor + (1.0 - eFactor) * surfaceColor;
      }

      void main()
      {
        vec3 color = edgeColor(surfaceColor());

        // Apply the stripy modulo effect
        float modVal = mod(Colorval, 2.0 * u_modLen);
        if(modVal > u_modLen) {
          color *= 0.7;
        }

        outputF = lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};

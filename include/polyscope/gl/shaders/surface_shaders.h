#pragma once

static const VertShader PLAIN_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
        {"a_barycoord", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      out vec3 Normal;
      out vec3 Position;
      out vec3 Barycoord;

      void main()
      {
          Position = a_position;
          Normal = a_normal;
          Barycoord = a_barycoord;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(Position,1.);
      }
    )
};

static const FragShader PLAIN_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_basecolor", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
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
      uniform vec3 u_eye;
      uniform vec3 u_lightCenter;
      uniform float u_lightDist;
      uniform float u_edgeWidth;
      uniform vec3 u_basecolor;
      in vec3 Normal;
      in vec3 Position;
      in vec3 Barycoord;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );
      float getEdgeFactor(vec3 UVW, float width);

      vec3 edgeColor(vec3 surfaceColor) {

          vec3 edgeColor = vec3(0.0, 0.0, 0.0);

          float eFactor = getEdgeFactor(Barycoord, u_edgeWidth);

          return eFactor * edgeColor + (1.0 - eFactor) * surfaceColor;
      }

      void main()
      {
        vec3 color = edgeColor(u_basecolor);
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};



static const VertShader VERTCOLOR_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
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
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in float a_colorval;
      out vec3 Normal;
      out vec3 Position;
      out vec3 Barycoord;
      out float Colorval;

      void main()
      {
          Position = a_position;
          Normal = a_normal;
          Barycoord = a_barycoord;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(Position,1.);
      }
    )
};

static const FragShader VERTCOLOR_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_basecolor", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
        {"u_edgeWidth", GLData::Float},
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
    GLSL(150,
      uniform vec3 u_eye;
      uniform vec3 u_lightCenter;
      uniform float u_lightDist;
      uniform float u_edgeWidth;
      uniform vec3 u_basecolor;
      uniform sampler1D t_colormap;
      in vec3 Normal;
      in vec3 Position;
      in vec3 Barycoord;
      in float Colorval;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );
      float getEdgeFactor(vec3 UVW, float width);

      vec3 surfaceColor() {
        // return Colorval * vec3(1., 0., 0.) + (1.0 - Colorval) * vec3(0., 1., 0.);
        return texture(t_colormap, Colorval).rgb;
      }

      vec3 edgeColor(vec3 surfaceColor) {

          vec3 edgeColor = vec3(0.0, 0.0, 0.0);

          float eFactor = getEdgeFactor(Barycoord, u_edgeWidth);

          return eFactor * edgeColor + (1.0 - eFactor) * surfaceColor;
      }

      void main()
      {
        vec3 color = edgeColor(surfaceColor());
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};

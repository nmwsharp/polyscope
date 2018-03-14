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
        {"u_rangeLow", GLData::Float},
        {"u_rangeHigh", GLData::Float},
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
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
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
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};


static const VertShader VERTBINARY_SURFACE_VERT_SHADER =  {
    
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
        {"a_colorval", GLData::Float}, // should be 0 or 1
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

static const FragShader VERTBINARY_SURFACE_FRAG_SHADER = {
    
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
        float t = 0.0;
        if(Colorval > 0.5) {
          t = 1.0;
        }
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
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};

static const VertShader VERTCOLOR3_SURFACE_VERT_SHADER =  {
    
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
        {"a_colorval", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in vec3 a_colorval;
      out vec3 Normal;
      out vec3 Position;
      out vec3 Barycoord;
      out vec3 Colorval;

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

static const FragShader VERTCOLOR3_SURFACE_FRAG_SHADER = {
    
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
      in vec3 Colorval;
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
        vec3 color = edgeColor(Colorval);
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};


static const VertShader HALFEDGECOLOR_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
        {"a_barycoord", GLData::Vector3Float},
        {"a_colorvals", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in vec3 a_colorvals;
      out vec3 Normal;
      out vec3 Position;
      out vec3 Barycoord;
      out vec3 Colorval;

      void main()
      {
          Position = a_position;
          Normal = a_normal;
          Barycoord = a_barycoord;
          Colorval = a_colorvals;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(Position,1.);
      }
    )
};

static const FragShader HALFEDGECOLOR_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_basecolor", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
        {"u_edgeWidth", GLData::Float},
        {"u_rangeLow", GLData::Float},
        {"u_rangeHigh", GLData::Float},
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
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform vec3 u_basecolor;
      uniform sampler1D t_colormap;
      in vec3 Normal;
      in vec3 Position;
      in vec3 Barycoord;
      in vec3 Colorval; // holds the value at the edge OPPOSITE vertex i
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );
      float getEdgeFactor(vec3 UVW, float width);

      vec3 surfaceColor() {

        // Blend by distance from edges
        vec3 eDist = (1.0 - Barycoord) / 2.0;
        float val = eDist.x * Colorval.x + eDist.y * Colorval.y +  eDist.z * Colorval.z;
        float t = (val - u_rangeLow) / (u_rangeHigh - u_rangeLow);
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
        outputF = lightSurface(Position, Normal, color, u_lightCenter, u_lightDist, u_eye);
      }

    )
};

static const VertShader PICK_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_barycoord", GLData::Vector3Float},
        {"a_vertexColors", GLData::Vector3Float, 3},
        {"a_edgeColors", GLData::Vector3Float, 3},
        {"a_halfedgeColors", GLData::Vector3Float, 3},
        {"a_faceColor", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;

      in vec3 a_position;
      in vec3 a_barycoord;

      in vec3 a_vertexColors[3];
      in vec3 a_edgeColors[3];
      in vec3 a_halfedgeColors[3];
      in vec3 a_faceColor;

      out vec3 Barycoord;
      
      flat out vec3 vertexColors[3];
      flat out vec3 edgeColors[3];
      flat out vec3 halfedgeColors[3];
      flat out vec3 faceColor;

      void main()
      {
          Barycoord = a_barycoord;

          for(int i = 0; i < 3; i++) {
              vertexColors[i] = a_vertexColors[i];
              edgeColors[i] = a_edgeColors[i];
              halfedgeColors[i] = a_halfedgeColors[i];
          }
          faceColor = a_faceColor;
          
          gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.);
      }
    )
};

static const FragShader PICK_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
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

      in vec3 Barycoord;
      
      flat in vec3 vertexColors[3];
      flat in vec3 edgeColors[3];
      flat in vec3 halfedgeColors[3];
      flat in vec3 faceColor;

      out vec4 outputF;


      void main()
      {

          // Parameters defining the pick shape (in barycentric 0-1 units)
          float vertRadius = 0.2;
          float edgeRadius = 0.1;
          float halfedgeRadius = 0.2;

          // Test vertices
          for(int i = 0; i < 3; i++) {
              if(Barycoord[i] > 1.0-vertRadius) {
                outputF = vec4(vertexColors[i], 1.0);
                return;
              }
          }

          // Test edges and halfedges
          for(int i = 0; i < 3; i++) {
              float eDist = Barycoord[(i+2)%3];
              if(eDist < edgeRadius) {
                outputF = vec4(edgeColors[i], 1.0);
                return;
              }
              if(eDist < halfedgeRadius) {
                outputF = vec4(halfedgeColors[i], 1.0);
                return;
              }
          }


          // If none of the above, fall back on the face
          outputF = vec4(faceColor, 1.0);
      }

    )
};



static const VertShader FACECOLOR_PLAIN_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_color", GLData::Vector3Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_color;
      flat out vec3 Colorval;

      void main()
      {
          Colorval = a_color;
          gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position,1.);
      }
    )
};

static const FragShader FACECOLOR_PLAIN_SURFACE_FRAG_SHADER = {
    
    // uniforms
    {
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
      flat in vec3 Colorval;
      out vec4 outputF;

      void main()
      {
        outputF = vec4(Colorval,1.0);
      }

    )
};

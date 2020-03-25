// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification PLAIN_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      out vec3 Normal;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification PLAIN_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_basecolor", DataType::Vector3Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform vec3 u_basecolor;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      in vec3 Normal;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      void main()
      {
        vec3 color = u_basecolor;
        outputF = vec4(lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};

const ShaderStageSpecification SURFACE_WIREFRAME_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    { // uniforms
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    { // attributes
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_barycoord", DataType::Vector3Float},
        {"a_edgeReal", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in vec3 a_edgeReal;
      out vec3 viewNormal;
      out vec3 Barycoord;
      out vec3 EdgeReal;

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          EdgeReal = a_edgeReal;
          Barycoord = a_barycoord;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification SURFACE_WIREFRAME_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    { // uniforms
        {"u_edgeColor", DataType::Vector3Float},
        {"u_edgeWidth", DataType::Float},
    }, 

    // attributes
    { },
    
    { // textures 
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_edgeWidth;
      uniform vec3 u_edgeColor;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      in vec3 viewNormal;
      in vec3 Barycoord;
      in vec3 EdgeReal;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

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

        vec4 outputColor = vec4(lightSurfaceMat(viewNormal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), alpha);
        outputF = outputColor;
      }

    )
};


const ShaderStageSpecification VERTCOLOR_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_colorval", DataType::Float},
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in float a_colorval;
      out vec3 Normal;
      out float Colorval;

      void main()
      {
          //Normal = a_normal;
          Normal = mat3(u_modelView) * a_normal;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification VERTCOLOR_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform sampler1D t_colormap;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      in vec3 Normal;
      in float Colorval;
      layout(location = 0) out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      float getEdgeFactor(vec3 UVW, float width);
      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      vec3 surfaceColor() {
        float t = (Colorval - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();
        outputF = vec4(lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};

/*

 const ShaderStageSpecification VERTBINARY_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_colorval", DataType::Float}, // should be 0 or 1
    },

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in float a_colorval;
      out vec3 viewNormal;
      out float colorVal;

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          colorVal = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

 const ShaderStageSpecification VERTBINARY_SURFACE_FRAG_SHADER = {
    
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
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      uniform sampler1D t_colormap;
      in vec3 viewNormal;
      in float colorVal;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
      float getEdgeFactor(vec3 UVW, float width);

      vec3 surfaceColor() {
        float t = 0.0;
        if(colorVal > 0.5) {
          t = 1.0;
        }
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();
        outputF = lightSurfaceMat(viewNormal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_b);
      }

    )
};

*/


const ShaderStageSpecification VERTCOLOR3_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_colorval", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_colorval;
      out vec3 Normal;
      out vec3 Colorval;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification VERTCOLOR3_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    { }, // uniforms

    { }, // attributes
    
    { // textures 
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      in vec3 Normal;
      in vec3 Colorval;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
      float getEdgeFactor(vec3 UVW, float width);

      void main()
      {
        vec3 color = Colorval;
        outputF = vec4(lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};


const ShaderStageSpecification HALFEDGECOLOR_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    { // uniforms
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    { // attributes
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_barycoord", DataType::Vector3Float},
        {"a_colorval", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec3 a_barycoord;
      in vec3 a_colorval;
      out vec3 Normal;
      out vec3 Barycoord;
      out vec3 Colorval;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Barycoord = a_barycoord;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification HALFEDGECOLOR_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      uniform sampler1D t_colormap;
      in vec3 Normal;
      in vec3 Barycoord;
      in vec3 Colorval; // holds the value at edge i --> i+1
      layout(location = 0) out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
      float getEdgeFactor(vec3 UVW, float width);

      vec3 surfaceColor() {

        // Blend by distance from edges
        //vec3 eDist = (1.0 - Barycoord) / 2.0;
        //float val = eDist.x * Colorval.x + eDist.y * Colorval.y +  eDist.z * Colorval.z;

        float val = Colorval.y;
        if(Barycoord.y < Barycoord.x && Barycoord.y < Barycoord.z) {
          val = Colorval.z;
        }
        if(Barycoord.z < Barycoord.x && Barycoord.z < Barycoord.y) {
          val = Colorval.x;
        }

        float t = (val - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();
        outputF = vec4(lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};



const ShaderStageSpecification PICK_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_barycoord", DataType::Vector3Float},
        {"a_vertexColors", DataType::Vector3Float, 3},
        {"a_edgeColors", DataType::Vector3Float, 3},
        {"a_halfedgeColors", DataType::Vector3Float, 3},
        {"a_faceColor", DataType::Vector3Float},
    },
    
    { }, // textures 

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;

      in vec3 a_position;
      in vec3 a_barycoord;

      in vec3 a_vertexColors[3];
      in vec3 a_edgeColors[3];
      in vec3 a_halfedgeColors[3];
      in vec3 a_faceColor;

      out vec3 barycoord;
      
      flat out vec3 vertexColors[3];
      flat out vec3 edgeColors[3];
      flat out vec3 halfedgeColors[3];
      flat out vec3 faceColor;

      void main()
      {
          barycoord = a_barycoord;

          for(int i = 0; i < 3; i++) {
              vertexColors[i] = a_vertexColors[i];
              edgeColors[i] = a_edgeColors[i];
              halfedgeColors[i] = a_halfedgeColors[i];
          }
          faceColor = a_faceColor;
          
          gl_Position = u_projMatrix * u_modelView * vec4(a_position, 1.);
      }
    )
};

const ShaderStageSpecification PICK_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    { }, // uniforms
    { }, // attributes 
    { }, // textures 

    // source 
    POLYSCOPE_GLSL(330 core,

      in vec3 barycoord;
      
      flat in vec3 vertexColors[3];
      flat in vec3 edgeColors[3];
      flat in vec3 halfedgeColors[3];
      flat in vec3 faceColor;

      layout(location = 0) out vec4 outputF;


      void main()
      {

          // Parameters defining the pick shape (in barycentric 0-1 units)
          float vertRadius = 0.2;
          float edgeRadius = 0.1;
          float halfedgeRadius = 0.2;

          // Test vertices
          for(int i = 0; i < 3; i++) {
              if(barycoord[i] > 1.0-vertRadius) {
                outputF = vec4(vertexColors[i], 1.0);
                return;
              }
          }

          // Test edges and halfedges
          for(int i = 0; i < 3; i++) {
              float eDist = barycoord[(i+2)%3];
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

const ShaderStageSpecification VERT_DIST_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_colorval", DataType::Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in float a_colorval;
      out vec3 Normal;
      out float Colorval;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Colorval = a_colorval;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification VERT_DIST_SURFACE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
        {"u_modLen", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform float u_modLen;
      uniform sampler1D t_colormap;
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      in vec3 Normal;
      in float Colorval;
      layout(location = 0) out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      vec3 surfaceColor() {
        float t = (Colorval - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();

        // Apply the stripy modulo effect
        float modVal = mod(Colorval, 2.0 * u_modLen);
        if(modVal > u_modLen) {
          color *= 0.7;
        }

        outputF = vec4(lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};

const ShaderStageSpecification PARAM_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_coord", DataType::Vector2Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec2 a_coord;
      out vec3 Normal;
      out vec2 Coord;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Coord = a_coord;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification PARAM_CHECKER_SURFACE_FRAG_SHADER = { 
  
    ShaderStageType::Fragment,
  
    // uniforms 
    {
        {"u_modLen", DataType::Float},
        {"u_color1", DataType::Vector3Float},
        {"u_color2", DataType::Vector3Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform vec3 u_color1;
      uniform vec3 u_color2;
      uniform float u_modLen;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;

      in vec3 Normal;
      in vec2 Coord;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      void main()
      {
        // Apply the checkerboard effect
        float mX = mod(Coord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(Coord.y, 2.0 * u_modLen) / u_modLen - 1.f;

        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        // TODO do some clever screen space derivative thing to prevent aliasing

        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;

        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(u_color1, u_color2, s);


        outputF = vec4(lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};



const ShaderStageSpecification PARAM_GRID_SURFACE_FRAG_SHADER = { 
  
    ShaderStageType::Fragment,
  
    // uniforms 
    {
        {"u_modLen", DataType::Float},
        {"u_gridLineColor", DataType::Vector3Float},
        {"u_gridBackgroundColor", DataType::Vector3Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform vec3 u_gridLineColor;
      uniform vec3 u_gridBackgroundColor;
      uniform float u_modLen;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;

      in vec3 Normal;
      in vec2 Coord;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      void main()
      {
        // Apply the checkerboard effect
        float mX = mod(Coord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(Coord.y, 2.0 * u_modLen) / u_modLen - 1.f;


        float minD = min(min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        
        float width = 0.05;
        float slopeWidthPix = 5.;

        vec2 fw = fwidth(Coord);
        float scale = max(fw.x, fw.y);
        float pWidth = slopeWidthPix * scale;

        float s = smoothstep(width, width + pWidth, minD);
        vec3 outColor = mix(u_gridLineColor, u_gridBackgroundColor, s);


        outputF = vec4(lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};


const ShaderStageSpecification PARAM_LOCAL_RAD_SURFACE_FRAG_SHADER = { 
  
    ShaderStageType::Fragment,
  
    // uniforms 
    {
        {"u_modLen", DataType::Float},
        {"u_angle", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_modLen;
      uniform float u_angle;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      uniform sampler1D t_colormap;

      in vec3 Normal;
      in vec2 Coord;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      void main()
      {

        // Get the color at this point
        float pi = 3.14159265359;
        float angle = atan(Coord.y, Coord.x) / (2. * pi) + 0.5; // in [0,1]
        float shiftedAngle = mod(angle + u_angle/(2. * pi), 1.);
        vec3 color = texture(t_colormap, shiftedAngle).rgb;
        vec3 colorDark = color * .5;

        // Apply the checkerboard effect (vert similar to rectangular checker
        float mX = mod(length(Coord), 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float minD = min(abs(mX), 1.0 - abs(mX)) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        float v = mX; // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(color, colorDark, s);

        outputF = vec4(lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};

const ShaderStageSpecification PARAM_LOCAL_CHECKER_SURFACE_FRAG_SHADER = { 
  
    ShaderStageType::Fragment,
  
    // uniforms 
    {
        {"u_modLen", DataType::Float},
        {"u_angle", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,
      uniform float u_modLen;
      uniform float u_angle;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler2D t_mat_k;
      uniform sampler1D t_colormap;

      in vec3 Normal;
      in vec2 Coord;
      layout(location = 0) out vec4 outputF;

      vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

      void main()
      {

        // Rotate coords
        float cosT = cos(u_angle);
        float sinT = sin(u_angle);
        vec2 rotCoord = vec2(cosT * Coord.x - sinT * Coord.y, sinT * Coord.x + cosT * Coord.y);
          
        // Get the color at this point
        float pi = 3.14159265359;
        float angle = atan(rotCoord.y, rotCoord.x) / (2. * pi) + 0.5; // in [0,1]
        vec3 color = texture(t_colormap, angle).rgb;
        vec3 colorDark = color * .5;

        // Apply the checkerboard effect (copied from checker above)
        float mX = mod(rotCoord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(rotCoord.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(color, colorDark, s);

        outputF = vec4(lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
      }

    )
};

/*
const ShaderStageSpecification FACECOLOR_PLAIN_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_color", DataType::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_color;
      flat out vec3 colorVal;

      void main()
      {
          colorVal = a_color;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

const ShaderStageSpecification FACECOLOR_PLAIN_SURFACE_FRAG_SHADER = {
    
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
    POLYSCOPE_GLSL(150,
      flat in vec3 colorVal;
      out vec4 outputF;

      void main()
      {
        outputF = vec4(colorVal,1.0);
      }

    )
};
*/

// clang-format on

} // namespace render
} // namespace polyscope

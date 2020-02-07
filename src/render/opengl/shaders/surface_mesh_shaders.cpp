// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification PLAIN_SURFACE_VERT_SHADER =  {
    
    // stage
    ShaderStageType::Vertex,
    
    { // uniforms
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    { // attributes
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
      out vec3 viewNormal;
      out vec3 viewPos;

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          vec4 viewPos4 = u_modelView * vec4(a_position,1.);
          viewPos = vec3(viewPos4);
          gl_Position = u_projMatrix * viewPos4;
      }
    )
};

const ShaderStageSpecification PLAIN_SURFACE_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_basecolor", DataType::Vector3Float},
        {"u_roughness", DataType::Float},
        {"u_metallic", DataType::Float},
        {"u_F0", DataType::Float},
    }, 

    // attributes
    { },
    
    // textures 
    { },
    
    // source 
    POLYSCOPE_GLSL_DEFERRED(330 core,
      uniform vec3 u_basecolor;
      uniform float u_roughness;
      uniform float u_metallic;
      uniform float u_F0;
      in vec3 viewNormal;
      in vec3 viewPos; 

      void main()
      {
        vec3 color = u_basecolor;

        gAlbedo = vec4(color, 1.);
        gMaterial = vec4(u_roughness, u_metallic, u_F0, 1.);
        gNormal = vec4(normalize(viewNormal), 1.);
        gPosition = vec4(viewPos, 1.);
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

      out vec3 viewNormal;
      out vec3 viewPos;
      out float colorVal; 

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          vec4 viewPos4 = u_modelView * vec4(a_position,1.);
          viewPos = vec3(viewPos4);
          gl_Position = u_projMatrix * viewPos4;
          colorVal = a_colorval;
      }
    )
};

const ShaderStageSpecification VERTCOLOR_SURFACE_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
        {"u_roughness", DataType::Float},
        {"u_metallic", DataType::Float},
        {"u_F0", DataType::Float},
    }, 

    {}, // attributes
    
    // textures 
    {
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL_DEFERRED(330 core,
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform float u_roughness;
      uniform float u_metallic;
      uniform float u_F0;
      uniform sampler1D t_colormap;
      in vec3 viewNormal;
      in vec3 viewPos; 
      in float colorVal;

      vec3 surfaceColor() {
        float t = (colorVal - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();
        
        gAlbedo = vec4(color, 1.);
        gMaterial = vec4(u_roughness, u_metallic, u_F0, 1.);
        gNormal = vec4(normalize(viewNormal), 1.);
        gPosition = vec4(viewPos, 1.);
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
      out vec3 Normal;
      out float colorVal;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
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
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler1D t_colormap;
      in vec3 Normal;
      in float colorVal;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
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
        outputF = lightSurfaceMat(Normal, color, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};

*/

const ShaderStageSpecification VERTCOLOR3_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    { // uniforms
       {"u_modelView", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    { // attributes
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
      out vec3 viewNormal;
      out vec3 viewPos;
      out vec3 colorVal; 

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          vec4 viewPos4 = u_modelView * vec4(a_position,1.);
          viewPos = vec3(viewPos4);
          gl_Position = u_projMatrix * viewPos4;
          colorVal = a_colorval;
      }
    )
};


const ShaderStageSpecification VERTCOLOR3_SURFACE_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_roughness", DataType::Float},
        {"u_metallic", DataType::Float},
        {"u_F0", DataType::Float},
    }, 

    {}, // attributes
    {}, // textures 
    
    // source 
    POLYSCOPE_GLSL_DEFERRED(330 core,
      uniform float u_roughness;
      uniform float u_metallic;
      uniform float u_F0;
      in vec3 viewNormal;
      in vec3 viewPos; 
      in vec3 colorVal;

      void main()
      {
        vec3 color = colorVal;
        
        gAlbedo = vec4(color, 1.);
        gMaterial = vec4(u_roughness, u_metallic, u_F0, 1.);
        gNormal = vec4(normalize(viewNormal), 1.);
        gPosition = vec4(viewPos, 1.);
      }

    )
};


const ShaderStageSpecification HALFEDGECOLOR_SURFACE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,

    { // uniforms
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
      out vec3 viewNormal;
      out vec3 viewPos;
      out vec3 colorVal; 
      out vec3 barycoord;

      void main()
      {
          viewNormal = mat3(u_modelView) * a_normal;
          vec4 viewPos4 = u_modelView * vec4(a_position,1.);
          viewPos = vec3(viewPos4);
          gl_Position = u_projMatrix * viewPos4;
          colorVal = a_colorval;
          barycoord = a_barycoord;
      }
    )
};

const ShaderStageSpecification HALFEDGECOLOR_SURFACE_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,

    { // uniforms
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
        {"u_roughness", DataType::Float},
        {"u_metallic", DataType::Float},
        {"u_F0", DataType::Float},
    }, 

    {}, // attributes
    
    // textures 
    {
        {"t_colormap", 1}
    },
    
    // source 
    POLYSCOPE_GLSL_DEFERRED(330 core,
      uniform float u_rangeLow;
      uniform float u_rangeHigh;
      uniform float u_roughness;
      uniform float u_metallic;
      uniform float u_F0;
      uniform sampler1D t_colormap;
      in vec3 viewNormal;
      in vec3 viewPos; 
      in vec3 barycoord;
      in vec3 colorVal; // holds the value at edge i --> i+1

      vec3 surfaceColor() {

        // Blend by distance from edges
        //vec3 eDist = (1.0 - barycoord) / 2.0;
        //float val = eDist.x * colorVal.x + eDist.y * colorVal.y +  eDist.z * colorVal.z;

        float val = colorVal.y;
        if(barycoord.y < barycoord.x && barycoord.y < barycoord.z) {
          val = colorVal.z;
        }
        if(barycoord.z < barycoord.x && barycoord.z < barycoord.y) {
          val = colorVal.x;
        }

        float t = (val - u_rangeLow) / (u_rangeHigh - u_rangeLow);
        t = clamp(t, 0.f, 1.f);
        return texture(t_colormap, t).rgb;
      }

      void main()
      {
        vec3 color = surfaceColor();

        gAlbedo = vec4(color, 1.);
        gMaterial = vec4(u_roughness, u_metallic, u_F0, 1.);
        gNormal = vec4(normalize(viewNormal), 1.);
        gPosition = vec4(viewPos, 1.);
      }

    )
};

/*

 const ShaderStageSpecification PICK_SURFACE_VERT_SHADER =  {
    
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
    
    // uniforms
    {
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
    
    // source 
    POLYSCOPE_GLSL(150,

      in vec3 barycoord;
      
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

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/shaders/surface_mesh_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification FLEX_MESH_VERT_SHADER = {

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
        {"a_barycoord", DataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        in vec3 a_position;
        in vec3 a_normal;
        in vec3 a_barycoord;
        out vec3 a_barycoordToFrag;
        out vec3 a_normalToFrag;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
            a_normalToFrag = mat3(u_modelView) * a_normal;
            a_barycoordToFrag = a_barycoord;

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_MESH_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$
        in vec3 a_normalToFrag;
        in vec3 a_barycoordToFrag;
        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        void main()
        {
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$
          
           // Shading
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$
           
           // Handle the wireframe
           ${ APPLY_WIREFRAME }$

           // Lighting
           vec3 shadeNormal = a_normalToFrag;
           ${ PERTURB_SHADE_NORMAL }$
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$
           
           // silly dummy usage to ensure normal and barycoords are always used; otherwise we get errors
           float dummyVal = a_normalToFrag.x + a_barycoordToFrag.x;
           alphaOut = alphaOut + dummyVal * (1e-12);

           ${ PERTURB_LIT_COLOR }$

           // Write output
           outputF = vec4(litColor, alphaOut);
        }
)"
};


// == Rules

// input: 2 uniforms and an int attribute
// output: vec3 albedoColor
const ShaderReplacementRule MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE (
    /* rule name */ "MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_faceColorType;
          out float a_faceColorTypeToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_faceColorTypeToFrag = a_faceColorType;
        )"},
      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_baseColor1;
          uniform vec3 u_baseColor2;
          in float a_faceColorTypeToFrag;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
          vec3 albedoColor = (a_faceColorTypeToFrag == 0.) ? u_baseColor1 : u_baseColor2;
        )"}
    },
    /* uniforms */ {
      {"u_baseColor1", DataType::Vector3Float},
      {"u_baseColor2", DataType::Vector3Float},
    },
    /* attributes */ {
      {"a_faceColorType", DataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_VALUE (
    /* rule name */ "MESH_PROPAGATE_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value;
          out float a_valueToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueToFrag = a_value;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value", DataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_HALFEDGE_VALUE (
    /* rule name */ "MESH_PROPAGATE_HALFEDGE_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_value3;
          out vec3 a_value3ToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_value3ToFrag = a_value3;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_value3ToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_value3ToFrag.y;
          if(a_barycoordToFrag.y < a_barycoordToFrag.x && a_barycoordToFrag.y < a_barycoordToFrag.z) {
            shadeValue = a_value3ToFrag.z;
          }
          if(a_barycoordToFrag.z < a_barycoordToFrag.x && a_barycoordToFrag.z < a_barycoordToFrag.y) {
            shadeValue = a_value3ToFrag.x;
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value3", DataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_COLOR (
    /* rule name */ "MESH_PROPAGATE_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color;
          out vec3 a_colorToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorToFrag = a_color;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_colorToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec3 shadeColor = a_colorToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color", DataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_VALUE2 (
    /* rule name */ "MESH_PROPAGATE_VALUE2",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec2 a_value2;
          out vec2 a_value2ToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_value2ToFrag = a_value2;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec2 a_value2ToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec2 shadeValue2 = a_value2ToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value2", DataType::Vector2Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_CULLPOS (
    /* rule name */ "MESH_PROPAGATE_CULLPOS",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_cullPos;
          out vec3 a_cullPosFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_cullPosFrag = vec3(u_modelView * vec4(a_cullPos, 1.));
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_cullPosFrag;
        )"},
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          vec3 cullPos = a_cullPosFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_cullPos", DataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_WIREFRAME(
    /* rule name */ "MESH_WIREFRAME",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_edgeIsReal;
          out vec3 a_edgeIsRealToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_edgeIsRealToFrag = a_edgeIsReal;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_edgeIsRealToFrag;

          uniform float u_edgeWidth;
          uniform vec3 u_edgeColor;
      
          float getEdgeFactor(vec3 UVW, vec3 edgeReal, float width) {
            // The Nick Sharp Edge Function. There are many like it, but this one is mine.
            float slopeWidth = 1.;
            
            vec3 fw = fwidth(UVW);
            vec3 realUVW = max(UVW, 1.0 - edgeReal.yzx);
            vec3 baryWidth = slopeWidth * fw;

            vec3 end = width*fw;
            vec3 dist = smoothstep(end - baryWidth, end, realUVW);

            float e = 1.0 - min(min(dist.x, dist.y), dist.z);
            return e;
          }
        )"},
      {"APPLY_WIREFRAME", R"(
          float edgeFactor = getEdgeFactor(a_barycoordToFrag, a_edgeIsRealToFrag, u_edgeWidth);
          albedoColor = mix(albedoColor, u_edgeColor, edgeFactor);
      )"},
    },
    /* uniforms */ {
      {"u_edgeColor", DataType::Vector3Float},
      {"u_edgeWidth", DataType::Float},
    },
    /* attributes */ {
      {"a_edgeIsReal", DataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_BACKFACE_NORMAL_FLIP (
    /* rule name */ "MESH_BACKFACE_NORMAL_FLIP",
    { /* replacement sources */
      {"PERTURB_SHADE_NORMAL", R"(
        if(!gl_FrontFacing) {
          shadeNormal *= -1.;
        }
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule MESH_BACKFACE_DARKEN (
    /* rule name */ "MESH_BACKFACE_DARKEN",
    { /* replacement sources */
      {"PERTURB_LIT_COLOR", R"(
        if(!gl_FrontFacing) {
          litColor *= .7;
        }
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


// data for picking
const ShaderReplacementRule MESH_PROPAGATE_PICK (
    /* rule name */ "MESH_PROPAGATE_PICK",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_vertexColors[3];
          in vec3 a_edgeColors[3];
          in vec3 a_halfedgeColors[3];
          in vec3 a_faceColor;
          flat out vec3 vertexColors[3];
          flat out vec3 edgeColors[3];
          flat out vec3 halfedgeColors[3];
          flat out vec3 faceColor;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          for(int i = 0; i < 3; i++) {
              vertexColors[i] = a_vertexColors[i];
              edgeColors[i] = a_edgeColors[i];
              halfedgeColors[i] = a_halfedgeColors[i];
          }
          faceColor = a_faceColor;
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 vertexColors[3];
          flat in vec3 edgeColors[3];
          flat in vec3 halfedgeColors[3];
          flat in vec3 faceColor;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          // Parameters defining the pick shape (in barycentric 0-1 units)
          float vertRadius = 0.2;
          float edgeRadius = 0.1;
          float halfedgeRadius = 0.2;
          
          vec3 shadeColor = faceColor;
          bool colorSet = false;

          // Test vertices
          for(int i = 0; i < 3; i++) {
              if(a_barycoordToFrag[i] > 1.0-vertRadius) {
                shadeColor = vertexColors[i];
                colorSet = true;
              }
          }

          // Test edges and halfedges
          for(int i = 0; i < 3; i++) {
              if(colorSet) continue;
              float eDist = a_barycoordToFrag[(i+2)%3];
              if(eDist < edgeRadius) {
                shadeColor = edgeColors[i];
                colorSet = true;
                continue;
              }
              if(eDist < halfedgeRadius) {
                shadeColor = halfedgeColors[i];
                colorSet = true;
              }
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_vertexColors", DataType::Vector3Float, 3},
      {"a_edgeColors", DataType::Vector3Float, 3},
      {"a_halfedgeColors", DataType::Vector3Float, 3},
      {"a_faceColor", DataType::Vector3Float},
    },
    /* textures */ {}
);


// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/surface_mesh_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3 {

// clang-format off

const ShaderStageSpecification FLEX_MESH_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_projMatrix", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_vertexPositions", RenderDataType::Vector3Float},
        {"a_vertexNormals", RenderDataType::Vector3Float},
        {"a_barycoord", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        
        in uint a_faceInds;
        
        in vec3 a_vertexPositions;
        in vec3 a_vertexNormals;
        in vec3 a_barycoord;
        out vec3 a_barycoordToFrag;
        out vec3 a_vertexNormalToFrag;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_projMatrix * u_modelView * vec4(a_vertexPositions,1.);
            
            a_vertexNormalToFrag = mat3(u_modelView) * a_vertexNormals;
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
        in vec3 a_vertexNormalToFrag;
        in vec3 a_barycoordToFrag;

        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        void main()
        {
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$
          
           // Shading
           vec3 shadeNormal = a_vertexNormalToFrag;
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$
           
           // Handle the wireframe
           ${ APPLY_WIREFRAME }$

           // Lighting
           ${ PERTURB_SHADE_NORMAL }$
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$
           
           ${ PERTURB_LIT_COLOR }$

           // Write output
           litColor *= alphaOut; // premultiplied alpha
           outputF = vec4(litColor, alphaOut);
        }
)"
};

const ShaderStageSpecification SIMPLE_MESH_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_projMatrix", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_vertexPositions", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        
        in vec3 a_vertexPositions;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_projMatrix * u_modelView * vec4(a_vertexPositions,1.);
            
            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification SIMPLE_MESH_FRAG_SHADER = {
    
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

        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        void main()
        {
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$
          
           // Shading
           vec3 shadeNormal = vec3(0., 0., 0.); // must be filled out by a rule below
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$

           // Lighting
           ${ PERTURB_SHADE_NORMAL }$
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$
           
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
      {"u_baseColor1", RenderDataType::Vector3Float},
      {"u_baseColor2", RenderDataType::Vector3Float},
    },
    /* attributes */ {
      {"a_faceColorType", RenderDataType::Float},
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
      {"a_value", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_VALUEALPHA (
    /* rule name */ "MESH_PROPAGATE_VALUEALPHA",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_valueAlpha;
          out float a_valueAlphaToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueAlphaToFrag = a_valueAlpha;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueAlphaToFrag;
        )"},
      {"GENERATE_ALPHA", R"(
          alphaOut *= clamp(a_valueAlphaToFrag, 0.f, 1.f);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_valueAlpha", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_FLAT_VALUE (
    /* rule name */ "MESH_PROPAGATE_FLAT_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value;
          flat out float a_valueToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueToFrag = a_value;
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in float a_valueToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_VALUE_CORNER_NEAREST (
    /* rule name */ "MESH_PROPAGATE_VALUE_CORNER_NEAREST",
    // REQUIRES: barycentric coords
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_value3;
          flat out vec3 a_value3ToFrag;
          out vec3 a_boostedBarycoordsToFrag;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_value3ToFrag = a_value3;

          // We want to slightly change the interpolation beyond nearest-vertex: if two
          // vertices in a triangle have the same value, we want to shade with a staight-line between
          // them, as if it's nearest to a shared linear function
          // This is a trick to get that behavior, by adjusting the value of the barycoords before
          // interpolation (can be shown to work by considering adding the post-interpolated coordinates 
          // if they match, then doing some algebra to pull it out before interpolation)
          vec3 boostedBarycoords = a_barycoord;
          if(a_value3.x == a_value3.y) {
            boostedBarycoords.x += a_barycoord.y;
            boostedBarycoords.y += a_barycoord.x;
          }
          if(a_value3.y == a_value3.z) {
            boostedBarycoords.y += a_barycoord.z;
            boostedBarycoords.z += a_barycoord.y;
          }
          if(a_value3.z == a_value3.x) {
            boostedBarycoords.z += a_barycoord.x;
            boostedBarycoords.x += a_barycoord.z;
          }

          // boostedBarycoords.y += a_barycoord.x * float(a_value3.x == a_value3.y);
          // boostedBarycoords.z += a_barycoord.x * float(a_value3.x == a_value3.z);
          // boostedBarycoords.x += a_barycoord.y * float(a_value3.y == a_value3.x);
          // boostedBarycoords.z += a_barycoord.y * float(a_value3.y == a_value3.z);
          // boostedBarycoords.x += a_barycoord.z * float(a_value3.z == a_value3.x);
          // boostedBarycoords.y += a_barycoord.z * float(a_value3.z == a_value3.y);

          a_boostedBarycoordsToFrag = boostedBarycoords;
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 a_value3ToFrag;
          in vec3 a_boostedBarycoordsToFrag;
          float selectMax(vec3 keys, vec3 values);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          // set the value equal to the entry of the vector corresponding to the largest component
          // of the barycoords
          float shadeValue = selectMax(a_boostedBarycoordsToFrag, a_value3ToFrag);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value3", RenderDataType::Vector3Float},
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
      {"a_value3", RenderDataType::Vector3Float},
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
      {"a_color", RenderDataType::Vector3Float},
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
      {"a_value2", RenderDataType::Vector2Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_TCOORD (
    /* rule name */ "MESH_PROPAGATE_TCOORD",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec2 a_tCoord;
          out vec2 tCoord;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          tCoord = a_tCoord;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec2 tCoord;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_tCoord", RenderDataType::Vector2Float},
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
      {"a_cullPos", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_WIREFRAME_FROM_BARY(
    /* rule name */ "MESH_WIREFRAME_FROM_BARY",
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
        )"},
      {"APPLY_WIREFRAME", R"(
          vec3 wireframe_UVW = a_barycoordToFrag;
          vec3 wireframe_mask = a_edgeIsRealToFrag;
      )"},
    },
    /* uniforms */ { },
    /* attributes */ {
      {"a_edgeIsReal", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_WIREFRAME(
    /* rule name */ "MESH_WIREFRAME",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(

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
          /* these wireframe_ should be populated by something else, such as the helper rule above */
          float edgeFactor = getEdgeFactor(wireframe_UVW, wireframe_mask, u_edgeWidth);
          albedoColor = mix(albedoColor, u_edgeColor, edgeFactor);
      )"},
    },
    /* uniforms */ {
      {"u_edgeColor", RenderDataType::Vector3Float},
      {"u_edgeWidth", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);


const ShaderReplacementRule MESH_WIREFRAME_ONLY( 
    // Must always be used in conjunction with MESH_WIREFRAME
    /* rule name */ "MESH_WIREFRAME_ONLY",
    { /* replacement sources */
      {"GENERATE_ALPHA", R"(
          alphaOut *= edgeFactor;
      )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
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


const ShaderReplacementRule MESH_BACKFACE_DIFFERENT (
    /* rule name */ "MESH_BACKFACE_DIFFERENT",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_backfaceColor;
        )"},
      {"GENERATE_SHADE_COLOR", R"(
        if(!gl_FrontFacing) {
          albedoColor = u_backfaceColor;
        }
  )"}
    },
    /* uniforms */ 
    {
      {"u_backfaceColor", RenderDataType::Vector3Float}
    },
    /* attributes */ {},
    /* textures */ {}
);


// data for picking
const ShaderReplacementRule MESH_PROPAGATE_PICK (
    /* rule name */ "MESH_PROPAGATE_PICK",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_vertexColors[3];
          in vec3 a_halfedgeColors[3];
          in vec3 a_cornerColors[3];
          in vec3 a_faceColor;
          flat out vec3 vertexColors[3];
          flat out vec3 halfedgeColors[3];
          flat out vec3 cornerColors[3];
          flat out vec3 faceColor;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          for(int i = 0; i < 3; i++) {
              vertexColors[i] = a_vertexColors[i];
              halfedgeColors[i] = a_halfedgeColors[i];
              cornerColors[i] = a_cornerColors[i];
          }
          faceColor = a_faceColor;
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 vertexColors[3];
          flat in vec3 halfedgeColors[3];
          flat in vec3 cornerColors[3];
          flat in vec3 faceColor;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          // Parameters defining the pick shape (in barycentric 0-1 units)
          float vertRadius = 0.15;
          float cornerRadius = 0.25;
          float halfedgeRadius = 0.15;
          
          vec3 shadeColor = faceColor;
          bool colorSet = false;

          // Test vertices and corners
          for(int i = 0; i < 3; i++) {
              if(a_barycoordToFrag[i] > 1.0-vertRadius) {
                shadeColor = vertexColors[i];
                colorSet = true;
                continue;
              }
              if(a_barycoordToFrag[i] > 1.0-cornerRadius) {
                shadeColor = cornerColors[i];
                colorSet = true;
              }
          }

          // Test halfedges
          for(int i = 0; i < 3; i++) {
              if(colorSet) continue;
              float eDist = a_barycoordToFrag[(i+2)%3];
              if(eDist < halfedgeRadius) {
                shadeColor = halfedgeColors[i];
                colorSet = true;
              }
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_vertexColors", RenderDataType::Vector3Float, 3},
      {"a_halfedgeColors", RenderDataType::Vector3Float, 3},
      {"a_cornerColors", RenderDataType::Vector3Float, 3},
      {"a_faceColor", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule MESH_PROPAGATE_PICK_SIMPLE ( // this one does faces and verts only
    /* rule name */ "MESH_PROPAGATE_PICK_SIMPLE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_vertexColors[3];
          in vec3 a_faceColor;
          flat out vec3 vertexColors[3];
          flat out vec3 faceColor;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          for(int i = 0; i < 3; i++) {
              vertexColors[i] = a_vertexColors[i];
          }
          faceColor = a_faceColor;
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 vertexColors[3];
          flat in vec3 faceColor;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          // Parameters defining the pick shape (in barycentric 0-1 units)
          float vertRadius = 0.2;
          
          vec3 shadeColor = faceColor;

          // Test vertices and corners
          for(int i = 0; i < 3; i++) {
              if(a_barycoordToFrag[i] > 1.0-vertRadius) {
                shadeColor = vertexColors[i];
              }
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_vertexColors", RenderDataType::Vector3Float, 3},
      {"a_faceColor", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);


// clang-format on

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

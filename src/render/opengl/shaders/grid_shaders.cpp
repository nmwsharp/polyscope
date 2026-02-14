// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/render/opengl/shaders/grid_shaders.h"


namespace polyscope {
namespace render {
namespace backend_openGL3 {

// clang-format off

const ShaderStageSpecification FLEX_GRIDCUBE_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
    }, 

    // attributes
    {
        {"a_cellPosition", RenderDataType::Vector3Float},
        {"a_cellInd", RenderDataType::Vector3Int},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_cellPosition;
        in ivec3 a_cellInd;
        
        out ivec3 a_cellIndToGeom;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = vec4(a_cellPosition, 1.);
            a_cellIndToGeom = a_cellInd;

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_GRIDCUBE_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_gridSpacing", RenderDataType::Vector3Float},
        {"u_cubeSizeFactor", RenderDataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;

        in ivec3 a_cellIndToGeom[];
        
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;

        uniform vec3 u_gridSpacing;
        uniform float u_cubeSizeFactor;

        out vec3 a_gridCoordToFrag;
        flat out ivec3 cellIndToFrag;
        out vec3 centerToFrag;

        ${ GEOM_DECLARATIONS }$

        void main() {

            vec3 center = gl_in[0].gl_Position.xyz;
            vec3 dvec = 0.5f * u_gridSpacing * u_cubeSizeFactor;

            mat4 T = u_projMatrix * u_modelView;
          
            // node corner logical +1/-1 offsets
            vec3 c0 = vec3(-1.f, -1.f, -1.f);
            vec3 c1 = vec3(-1.f, -1.f,  1.f);
            vec3 c2 = vec3(-1.f,  1.f, -1.f);
            vec3 c3 = vec3(-1.f,  1.f,  1.f);
            vec3 c4 = vec3( 1.f, -1.f, -1.f);
            vec3 c5 = vec3( 1.f, -1.f,  1.f);
            vec3 c6 = vec3( 1.f,  1.f, -1.f);
            vec3 c7 = vec3( 1.f,  1.f,  1.f);

            // node corner positions
            vec4 p0 = T * vec4(center + c0 * dvec, 1.f);
            vec4 p1 = T * vec4(center + c1 * dvec, 1.f);
            vec4 p2 = T * vec4(center + c2 * dvec, 1.f);
            vec4 p3 = T * vec4(center + c3 * dvec, 1.f);
            vec4 p4 = T * vec4(center + c4 * dvec, 1.f);
            vec4 p5 = T * vec4(center + c5 * dvec, 1.f);
            vec4 p6 = T * vec4(center + c6 * dvec, 1.f);
            vec4 p7 = T * vec4(center + c7 * dvec, 1.f);

            // node corner indices
            ivec3 cellInd = a_cellIndToGeom[0];
            ivec3 i0 = cellInd + ivec3(0, 0, 0);
            ivec3 i1 = cellInd + ivec3(0, 0, 1);
            ivec3 i2 = cellInd + ivec3(0, 1, 0);
            ivec3 i3 = cellInd + ivec3(0, 1, 1);
            ivec3 i4 = cellInd + ivec3(1, 0, 0);
            ivec3 i5 = cellInd + ivec3(1, 0, 1);
            ivec3 i6 = cellInd + ivec3(1, 1, 0);
            ivec3 i7 = cellInd + ivec3(1, 1, 1);
            
            ${ GEOM_COMPUTE_BEFORE_EMIT }$

            vec4 nodePos;
            ivec3 nodeInd;
            
            // this is the order to emit vertices to get a cube triangle strip
            // 3, 7, 1, 5, 4, 7, 6, 3, 2, 1, 0, 4, 2, 6,

            /* 7 */ nodePos = p7; nodeInd = i7; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c7; EmitVertex(); 
            /* 3 */ nodePos = p3; nodeInd = i3; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c3; EmitVertex(); 
            /* 5 */ nodePos = p5; nodeInd = i5; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c5; EmitVertex(); 
            /* 1 */ nodePos = p1; nodeInd = i1; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c1; EmitVertex(); 
            /* 0 */ nodePos = p0; nodeInd = i0; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c0; EmitVertex(); 
            /* 3 */ nodePos = p3; nodeInd = i3; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c3; EmitVertex(); 
            /* 2 */ nodePos = p2; nodeInd = i2; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c2; EmitVertex(); 
            /* 7 */ nodePos = p7; nodeInd = i7; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c7; EmitVertex(); 
            /* 6 */ nodePos = p6; nodeInd = i6; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c6; EmitVertex(); 
            /* 5 */ nodePos = p5; nodeInd = i5; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c5; EmitVertex(); 
            /* 4 */ nodePos = p4; nodeInd = i4; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c4; EmitVertex(); 
            /* 0 */ nodePos = p0; nodeInd = i0; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c0; EmitVertex(); 
            /* 6 */ nodePos = p6; nodeInd = i6; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c6; EmitVertex(); 
            /* 2 */ nodePos = p2; nodeInd = i2; centerToFrag = center; cellIndToFrag = cellInd; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; a_gridCoordToFrag = c2; EmitVertex(); 

            EndPrimitive();

        }

)"
};

const ShaderStageSpecification FLEX_GRIDCUBE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_gridCoordToFrag;
        flat in ivec3 cellIndToFrag;
        in vec3 centerToFrag;
        uniform mat4 u_modelView;
        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        vec3 sharpenToAxis(vec3 v, float sharpness);

        void main()
        {
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$

           vec3 coordLocalAbs = abs(a_gridCoordToFrag);
           float maxCoord = max(max(coordLocalAbs.x, coordLocalAbs.y), coordLocalAbs.z);
           vec3 cellInd3f = vec3(cellIndToFrag);
          
           // compute a normal vector from the coord
           vec3 shadeNormal = sharpenToAxis(a_gridCoordToFrag, 8.0f);
           shadeNormal = normalize(mat3(u_modelView) * shadeNormal); // transform to view space
          
           // Shading
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

const ShaderStageSpecification FLEX_GRIDCUBE_PLANE_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_boundMin", RenderDataType::Vector3Float},
        {"u_boundMax", RenderDataType::Vector3Float},
        {"u_cubeSizeFactor", RenderDataType::Float},
        {"u_gridSpacingReference", RenderDataType::Vector3Float},
    }, 

    // attributes
    {
        {"a_referencePosition", RenderDataType::Vector3Float},
        {"a_referenceNormal", RenderDataType::Vector3Float},
        {"a_axisInd", RenderDataType::Int},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$
        
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform vec3 u_boundMin;
        uniform vec3 u_boundMax;
        uniform float u_cubeSizeFactor;
        uniform vec3 u_gridSpacingReference;

        in vec3 a_referencePosition;
        in vec3 a_referenceNormal;
        in int a_axisInd;
        
        out vec3 a_coordToFrag;
        out vec3 a_normalToFrag;
        out vec3 a_refNormalToFrag;
        flat out int a_axisIndToFrag;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {

            // first apply any scale shrinking 
            vec3 startPos = a_referencePosition;
            vec3 adjPosition = a_referencePosition - a_referenceNormal * (1.f - (0.5 + u_cubeSizeFactor/2.)) * u_gridSpacingReference;

            // apply box shift
            vec3 boxPos = mix(u_boundMin, u_boundMax, adjPosition);

            a_coordToFrag = adjPosition;
            a_normalToFrag = mat3(u_modelView) * a_referenceNormal;
            a_refNormalToFrag = a_referenceNormal;
            a_axisIndToFrag = a_axisInd;
            gl_Position = u_projMatrix * u_modelView * vec4(boxPos,1.);

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_GRIDCUBE_PLANE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_gridSpacingReference", RenderDataType::Vector3Float},
        {"u_cubeSizeFactor", RenderDataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$
        
        in vec3 a_coordToFrag;
        in vec3 a_normalToFrag;
        in vec3 a_refNormalToFrag;
        flat in int a_axisIndToFrag;
        
        uniform vec3 u_gridSpacingReference;
        uniform float u_cubeSizeFactor;

        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        void main()
        {
           float REF_EPS = 0.0001;
           
           // do some coordinate arithmetic
           // NOTE: this logic is duplicated with pick function
           vec3 coordUnit = a_coordToFrag / u_gridSpacingReference;
           vec3 coordMod = mod(coordUnit, 1.f); // [0,1] within each cell
           vec3 coordModShift = 2.f*coordMod - 1.f; // [-1,1] within each cell
           vec3 coordLocal = coordModShift / u_cubeSizeFactor; // [-1,1] within each scaled cell
           vec3 coordLocalAbs = abs(coordLocal) * (1.f - abs(a_refNormalToFrag));
           float maxCoord = max(max(coordLocalAbs.x, coordLocalAbs.y), coordLocalAbs.z);

           vec3 cellInd3f = floor(coordUnit - REF_EPS*a_refNormalToFrag);
           uvec3 cellInd = uvec3(cellInd3f);

           // discard the gaps in the cubes
           if(maxCoord > 1.f + REF_EPS) { // note the threshold here, hacky but seems okay
             discard;
           }
           
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$

           // == test visibility for this and neighbor
           // as an optimization, we discard faces of cubes which will no be visible: if a cube and 
           // its neighbor are both visible, there is no need to render a face between them
           // (note that this actually means almost all faces get discarded!)
           if(u_cubeSizeFactor == 1.f) { // don't discard if there are gaps between the cubes
             vec3 neighCoordUnit = a_coordToFrag + a_refNormalToFrag * u_gridSpacingReference;
             bool neighIsVisible = (all(greaterThan(neighCoordUnit, vec3(-REF_EPS))) && all(lessThan(neighCoordUnit, vec3(1.f + REF_EPS))));

             // catch additional neighbors which are visible due to slice planes
             ${ GRID_PLANE_NEIGHBOR_FILTER }$

             if(neighIsVisible) {
               discard;
             }
           }
           

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
           
           ${ PERTURB_LIT_COLOR }$


           // Write output
           litColor *= alphaOut; // premultiplied alpha
           outputF = vec4(litColor, alphaOut);
        }
)"
};


const ShaderReplacementRule GRIDCUBE_PROPAGATE_NODE_VALUE (
    /* rule name */ "GRIDCUBE_PROPAGATE_NODE_VALUE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler3D t_value;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = texture(t_value, a_coordToFrag).r;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ { },
    /* textures */ {
      {"t_value", 3},
    }
);

const ShaderReplacementRule GRIDCUBE_PROPAGATE_CELL_VALUE (
    /* rule name */ "GRIDCUBE_PROPAGATE_CELL_VALUE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler3D t_value;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = texelFetch(t_value, ivec3(cellInd), 0).r;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ { },
    /* textures */ {
      {"t_value", 3},
    }
);

const ShaderReplacementRule GRIDCUBE_WIREFRAME (
    /* rule name */ "GRIDCUBE_WIREFRAME",
    {
        /* replacement sources */
        {"APPLY_WIREFRAME", R"(
           vec3 wireframe_UVW = 1.f - coordLocalAbs;
           vec3 wireframe_mask = vec3(notEqual(abs(a_gridCoordToFrag), vec3(1.f, 1.f, 1.f))).zxy;
      )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_PLANE_WIREFRAME (
    /* rule name */ "GRIDCUBE_PLANE_WIREFRAME",
    {
        /* replacement sources */
        {"APPLY_WIREFRAME", R"(
          vec3 wireframe_UVW = 1.f - coordLocalAbs;
          vec3 wireframe_mask = (1.f - abs(a_refNormalToFrag)).zxy;
      )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_CONSTANT_PICK(
    /* rule name */ "GRIDCUBE_CONSTANT_PICK",
    {
        /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_pickColor;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec3 shadeColor = u_pickColor;
        )"},
    },
    /* uniforms */ {
      {"u_pickColor", RenderDataType::Vector3Float}
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_CULLPOS_FROM_CENTER(
    /* rule name */ "GRIDCUBE_CULLPOS_FROM_CENTER",
    { /* replacement sources */

      {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_gridSpacing;
        )"},
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          // NOTE: you would expect the constant below to be 0.f, to cull from the center of the cell. 
          // We intentionally use 0.167 instead and slightly shift it, to avoid common default causes 
          // where the plane slices right through the center of the cell, and you get random patterns 
          // of cull/not-cull based on floating point error.
          const float cull_shift = 0.167;
          vec3 cullPos = (u_modelView * vec4(centerToFrag + cull_shift * u_gridSpacing, 1.f)).xyz;
        )"},
    },
    /* uniforms */ {
      {"u_gridSpacing", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_PLANE_CULLPOS_FROM_CENTER(
    /* rule name */ "GRIDCUBE_PLANE_CULLPOS_FROM_CENTER",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform mat4 u_modelView;
          uniform vec3 u_boundMin;
          uniform vec3 u_boundMax;
        )"},
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          // NOTE: you would expect the constant below to be 0.f, to cull from the center of the cell. 
          // We intentionally use 0.167 instead and slightly shift it, to avoid common default causes 
          // where the plane slices right through the center of the cell, and you get random patterns 
          // of cull/not-cull based on floating point error.
          const float cull_shift = 0.167;
          vec3 cullPosRef = (0.5f + cull_shift + cellInd3f) * u_gridSpacingReference;
          vec3 cullPosWorld = mix(u_boundMin, u_boundMax, cullPosRef);
          vec3 cullPos = (u_modelView * vec4(cullPosWorld, 1.f)).xyz;
         
          // compute the same data for the neighboring cell too
          // we need this due to the neighbor visibiilty filtering
          vec3 neighCullPosRef = (0.5f + cull_shift + cellInd3f + a_refNormalToFrag) * u_gridSpacingReference;
          vec3 neighCullPosWorld = mix(u_boundMin, u_boundMax, neighCullPosRef);
          vec3 neighCullPos = (u_modelView * vec4(neighCullPosWorld, 1.f)).xyz;
        )"},
    },
    /* uniforms */ {
      {"u_modelView", RenderDataType::Matrix44Float},
      {"u_boundMin", RenderDataType::Vector3Float},
      {"u_boundMax", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

// == Attribute-based rules for sparse volume grid quantities ==

const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR (
    /* rule name */ "GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value;
          out float a_valueToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueToGeom = a_value;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_valueToGeom[];
          out float a_valueToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_valueToFrag = a_valueToGeom[0];
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

const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR (
    /* rule name */ "GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color;
          out vec3 a_colorToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorToGeom = a_color;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorToGeom[];
          flat out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorToFrag = a_colorToGeom[0];
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 a_colorToFrag;
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

const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR (
    /* rule name */ "GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_nodeValue0;
          in float a_nodeValue1;
          in float a_nodeValue2;
          in float a_nodeValue3;
          in float a_nodeValue4;
          in float a_nodeValue5;
          in float a_nodeValue6;
          in float a_nodeValue7;
          out float a_nodeValue0ToGeom;
          out float a_nodeValue1ToGeom;
          out float a_nodeValue2ToGeom;
          out float a_nodeValue3ToGeom;
          out float a_nodeValue4ToGeom;
          out float a_nodeValue5ToGeom;
          out float a_nodeValue6ToGeom;
          out float a_nodeValue7ToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_nodeValue0ToGeom = a_nodeValue0;
          a_nodeValue1ToGeom = a_nodeValue1;
          a_nodeValue2ToGeom = a_nodeValue2;
          a_nodeValue3ToGeom = a_nodeValue3;
          a_nodeValue4ToGeom = a_nodeValue4;
          a_nodeValue5ToGeom = a_nodeValue5;
          a_nodeValue6ToGeom = a_nodeValue6;
          a_nodeValue7ToGeom = a_nodeValue7;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_nodeValue0ToGeom[];
          in float a_nodeValue1ToGeom[];
          in float a_nodeValue2ToGeom[];
          in float a_nodeValue3ToGeom[];
          in float a_nodeValue4ToGeom[];
          in float a_nodeValue5ToGeom[];
          in float a_nodeValue6ToGeom[];
          in float a_nodeValue7ToGeom[];
          out float a_valueToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          {
            int cornerIdx = (nodeInd.x - cellInd.x) * 4 + (nodeInd.y - cellInd.y) * 2 + (nodeInd.z - cellInd.z);
            float vals[8] = float[8](
              a_nodeValue0ToGeom[0], a_nodeValue1ToGeom[0], a_nodeValue2ToGeom[0], a_nodeValue3ToGeom[0],
              a_nodeValue4ToGeom[0], a_nodeValue5ToGeom[0], a_nodeValue6ToGeom[0], a_nodeValue7ToGeom[0]
            );
            a_valueToFrag = vals[cornerIdx];
          }
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
      {"a_nodeValue0", RenderDataType::Float},
      {"a_nodeValue1", RenderDataType::Float},
      {"a_nodeValue2", RenderDataType::Float},
      {"a_nodeValue3", RenderDataType::Float},
      {"a_nodeValue4", RenderDataType::Float},
      {"a_nodeValue5", RenderDataType::Float},
      {"a_nodeValue6", RenderDataType::Float},
      {"a_nodeValue7", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR (
    /* rule name */ "GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_nodeColor0;
          in vec3 a_nodeColor1;
          in vec3 a_nodeColor2;
          in vec3 a_nodeColor3;
          in vec3 a_nodeColor4;
          in vec3 a_nodeColor5;
          in vec3 a_nodeColor6;
          in vec3 a_nodeColor7;
          out vec3 a_nodeColor0ToGeom;
          out vec3 a_nodeColor1ToGeom;
          out vec3 a_nodeColor2ToGeom;
          out vec3 a_nodeColor3ToGeom;
          out vec3 a_nodeColor4ToGeom;
          out vec3 a_nodeColor5ToGeom;
          out vec3 a_nodeColor6ToGeom;
          out vec3 a_nodeColor7ToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_nodeColor0ToGeom = a_nodeColor0;
          a_nodeColor1ToGeom = a_nodeColor1;
          a_nodeColor2ToGeom = a_nodeColor2;
          a_nodeColor3ToGeom = a_nodeColor3;
          a_nodeColor4ToGeom = a_nodeColor4;
          a_nodeColor5ToGeom = a_nodeColor5;
          a_nodeColor6ToGeom = a_nodeColor6;
          a_nodeColor7ToGeom = a_nodeColor7;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_nodeColor0ToGeom[];
          in vec3 a_nodeColor1ToGeom[];
          in vec3 a_nodeColor2ToGeom[];
          in vec3 a_nodeColor3ToGeom[];
          in vec3 a_nodeColor4ToGeom[];
          in vec3 a_nodeColor5ToGeom[];
          in vec3 a_nodeColor6ToGeom[];
          in vec3 a_nodeColor7ToGeom[];
          out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          {
            int cornerIdx = (nodeInd.x - cellInd.x) * 4 + (nodeInd.y - cellInd.y) * 2 + (nodeInd.z - cellInd.z);
            vec3 cols[8] = vec3[8](
              a_nodeColor0ToGeom[0], a_nodeColor1ToGeom[0], a_nodeColor2ToGeom[0], a_nodeColor3ToGeom[0],
              a_nodeColor4ToGeom[0], a_nodeColor5ToGeom[0], a_nodeColor6ToGeom[0], a_nodeColor7ToGeom[0]
            );
            a_colorToFrag = cols[cornerIdx];
          }
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
      {"a_nodeColor0", RenderDataType::Vector3Float},
      {"a_nodeColor1", RenderDataType::Vector3Float},
      {"a_nodeColor2", RenderDataType::Vector3Float},
      {"a_nodeColor3", RenderDataType::Vector3Float},
      {"a_nodeColor4", RenderDataType::Vector3Float},
      {"a_nodeColor5", RenderDataType::Vector3Float},
      {"a_nodeColor6", RenderDataType::Vector3Float},
      {"a_nodeColor7", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

}
}
}

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
        {"a_cellInd", RenderDataType::Vector3UInt},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_cellPosition;
        in uvec3 a_cellInd;
        
        out uvec3 a_cellIndToGeom;
        
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

        in uvec3 a_cellIndToGeom[];
        
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;

        uniform vec3 u_gridSpacing;
        uniform float u_cubeSizeFactor;

        out vec3 sphereCenterView;

        ${ GEOM_DECLARATIONS }$

        void main() {

            vec3 center = gl_in[0].gl_Position.xyz;
            vec3 dvec = 0.5f * u_gridSpacing * u_cubeSizeFactor;

            mat4 T = u_projMatrix * u_modelView;
          
            // node corner positions
            vec4 p0 = T * vec4(center + vec3(-1.f, -1.f, -1.f)*dvec, 1.f);
            vec4 p1 = T * vec4(center + vec3(-1.f, -1.f,  1.f)*dvec, 1.f);
            vec4 p2 = T * vec4(center + vec3(-1.f,  1.f, -1.f)*dvec, 1.f);
            vec4 p3 = T * vec4(center + vec3(-1.f,  1.f,  1.f)*dvec, 1.f);
            vec4 p4 = T * vec4(center + vec3( 1.f, -1.f, -1.f)*dvec, 1.f);
            vec4 p5 = T * vec4(center + vec3( 1.f, -1.f,  1.f)*dvec, 1.f);
            vec4 p6 = T * vec4(center + vec3( 1.f,  1.f, -1.f)*dvec, 1.f);
            vec4 p7 = T * vec4(center + vec3( 1.f,  1.f,  1.f)*dvec, 1.f);

            // node corner indices
            uvec3 iCenter = a_cellIndToGeom[0];
            uvec3 i0 = iCenter + uvec3(0, 0, 0);
            uvec3 i1 = iCenter + uvec3(0, 0, 1);
            uvec3 i2 = iCenter + uvec3(0, 1, 0);
            uvec3 i3 = iCenter + uvec3(0, 1, 1);
            uvec3 i4 = iCenter + uvec3(1, 0, 0);
            uvec3 i5 = iCenter + uvec3(1, 0, 1);
            uvec3 i6 = iCenter + uvec3(1, 1, 0);
            uvec3 i7 = iCenter + uvec3(1, 1, 1);
            
            ${ GEOM_COMPUTE_BEFORE_EMIT }$

            vec4 nodePos;
            uvec3 nodeInd;
            uvec3 cellInd;
            
            // this is the order to emit veritces to get a cube triangle strip
            // 3, 7, 1, 5, 4, 7, 6, 3, 2, 1, 0, 4, 2, 6,

            /* 7 */ nodePos = p7; nodeInd = i7; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 3 */ nodePos = p3; nodeInd = i3; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 5 */ nodePos = p5; nodeInd = i5; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 1 */ nodePos = p1; nodeInd = i1; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 0 */ nodePos = p0; nodeInd = i0; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 3 */ nodePos = p3; nodeInd = i3; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 2 */ nodePos = p2; nodeInd = i2; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 7 */ nodePos = p7; nodeInd = i7; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 6 */ nodePos = p6; nodeInd = i6; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 5 */ nodePos = p5; nodeInd = i5; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 4 */ nodePos = p4; nodeInd = i4; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 0 */ nodePos = p0; nodeInd = i0; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 6 */ nodePos = p6; nodeInd = i6; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 
            /* 2 */ nodePos = p2; nodeInd = i2; cellInd = iCenter; ${ GEOM_PER_EMIT }$ gl_Position = nodePos; EmitVertex(); 

            EndPrimitive();

        }

)"
};

const ShaderStageSpecification FLEX_GRIDCUBE_FRAG_SHADER = {
    
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
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$
           
           // Handle the wireframe
           ${ APPLY_WIREFRAME }$

           // Lighting
           vec3 shadeNormal = vec3(0.f, 0.f, 0.f); // use the COMPUTE_SHADE_NORMAL_FROM_POSITION rule
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
          in vec4 a_nodeValues04;
          in vec4 a_nodeValues47;
          out vec4 a_nodeValues04ToGeom;
          out vec4 a_nodeValues47ToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_nodeValues04ToGeom = a_nodeValues04;
          a_nodeValues47ToGeom = a_nodeValues47;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec4 a_nodeValues04ToGeom[];
          in vec4 a_nodeValues47ToGeom[];
          out float a_valueToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          {
            uint cornerIdx = (nodeInd.x - cellInd.x) * 4u + (nodeInd.y - cellInd.y) * 2u + (nodeInd.z - cellInd.z);
            a_valueToFrag = (cornerIdx < 4u) ? a_nodeValues04ToGeom[0][cornerIdx] : a_nodeValues47ToGeom[0][cornerIdx - 4u];
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
      {"a_nodeValues04", RenderDataType::Vector4Float},
      {"a_nodeValues47", RenderDataType::Vector4Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR (
    /* rule name */ "GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec4 a_nodeR04;
          in vec4 a_nodeR47;
          in vec4 a_nodeG04;
          in vec4 a_nodeG47;
          in vec4 a_nodeB04;
          in vec4 a_nodeB47;
          out vec4 a_nodeR04ToGeom;
          out vec4 a_nodeR47ToGeom;
          out vec4 a_nodeG04ToGeom;
          out vec4 a_nodeG47ToGeom;
          out vec4 a_nodeB04ToGeom;
          out vec4 a_nodeB47ToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_nodeR04ToGeom = a_nodeR04;
          a_nodeR47ToGeom = a_nodeR47;
          a_nodeG04ToGeom = a_nodeG04;
          a_nodeG47ToGeom = a_nodeG47;
          a_nodeB04ToGeom = a_nodeB04;
          a_nodeB47ToGeom = a_nodeB47;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec4 a_nodeR04ToGeom[];
          in vec4 a_nodeR47ToGeom[];
          in vec4 a_nodeG04ToGeom[];
          in vec4 a_nodeG47ToGeom[];
          in vec4 a_nodeB04ToGeom[];
          in vec4 a_nodeB47ToGeom[];
          out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          {
            uint cornerIdx = (nodeInd.x - cellInd.x) * 4u + (nodeInd.y - cellInd.y) * 2u + (nodeInd.z - cellInd.z);
            float r = (cornerIdx < 4u) ? a_nodeR04ToGeom[0][cornerIdx] : a_nodeR47ToGeom[0][cornerIdx - 4u];
            float g = (cornerIdx < 4u) ? a_nodeG04ToGeom[0][cornerIdx] : a_nodeG47ToGeom[0][cornerIdx - 4u];
            float b = (cornerIdx < 4u) ? a_nodeB04ToGeom[0][cornerIdx] : a_nodeB47ToGeom[0][cornerIdx - 4u];
            a_colorToFrag = vec3(r, g, b);
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
      {"a_nodeR04", RenderDataType::Vector4Float},
      {"a_nodeR47", RenderDataType::Vector4Float},
      {"a_nodeG04", RenderDataType::Vector4Float},
      {"a_nodeG47", RenderDataType::Vector4Float},
      {"a_nodeB04", RenderDataType::Vector4Float},
      {"a_nodeB47", RenderDataType::Vector4Float},
    },
    /* textures */ {}
);

}
}
}

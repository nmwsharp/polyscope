// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/render/opengl/shaders/grid_shaders.h"


namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

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
           vec3 shadeNormal = vec3(0.f, 0.f, 0.f); // use the MESH_COMPUTE_NORMAL_FROM_POSITION rule
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


}
}
}

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/ribbon_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification RIBBON_VERT_SHADER = {

    ShaderStageType::Vertex,

    { }, // uniforms

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
        {"a_color", RenderDataType::Vector3Float},
        {"a_normal", RenderDataType::Vector3Float},
    },
    
    {}, // textures

    // source
R"(
       ${ GLSL_VERSION }$

        in vec3 a_position;
        in vec3 a_color;
        in vec3 a_normal;
        out vec3 Color;
        out vec3 Normal;
        void main()
        {
            Color = a_color;
            Normal = a_normal;
            gl_Position = vec4(a_position,1.0);
        }
)"
};


const ShaderStageSpecification RIBBON_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_ribbonWidth", RenderDataType::Float},
        {"u_depthOffset", RenderDataType::Float},
    }, 

    // attributes
    {
    },
    
    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        layout(lines_adjacency) in;
        layout(triangle_strip, max_vertices=20) out;
        in vec3 Color[];
        in vec3 Normal[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_ribbonWidth;
        uniform float u_depthOffset;
        out vec3 colorToFrag;
        out vec3 cameraNormalToFrag;
        out float intensityToFrag;
        void main()   {
            mat4 PV = u_projMatrix * u_modelView;
            const float PI = 3.14159265358;

            vec3 pos0 = gl_in[0].gl_Position.xyz;
            vec3 pos1 = gl_in[1].gl_Position.xyz;
            vec3 pos2 = gl_in[2].gl_Position.xyz;
            vec3 pos3 = gl_in[3].gl_Position.xyz;
            vec3 dir = normalize(pos2 - pos1);
            vec3 prevDir = normalize(pos1 - pos0);
            vec3 nextDir = normalize(pos3 - pos2);
            vec3 sideVec0 = normalize(cross(normalize(dir + prevDir), Normal[1]));
            vec3 sideVec1 = normalize(cross(normalize(dir + nextDir), Normal[2]));

            // The points on the front and back sides of the ribbon
            vec4 pStartLeft = vec4(pos1 + sideVec0 * u_ribbonWidth, 1);
            vec4 pStartMid = vec4(pos1, 1);
            vec4 pStartRight = vec4(pos1 - sideVec0 * u_ribbonWidth, 1);
            vec4 pEndLeft = vec4(pos2 + sideVec1 * u_ribbonWidth, 1);
            vec4 pEndMid = vec4(pos2, 1);
            vec4 pEndRight = vec4(pos2 - sideVec1 * u_ribbonWidth, 1);

            // First triangle
            gl_Position = PV * pStartRight;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[1];
            colorToFrag = Color[1];
            intensityToFrag = 0.0;
            EmitVertex();
            
            gl_Position = PV * pEndRight;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[2];
            colorToFrag = Color[2];
            intensityToFrag = 0.0;
            EmitVertex();
            
            gl_Position = PV * pStartMid;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[1];
            colorToFrag = Color[1];
            intensityToFrag = 1.0;
            EmitVertex();

            // Second triangle
            gl_Position = PV * pEndMid;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[2];
            colorToFrag = Color[2];
            intensityToFrag = 1.0;
            EmitVertex();

            // Third triangle
            gl_Position = PV * pStartLeft;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[1];
            colorToFrag = Color[1];
            intensityToFrag = 0.0;
            EmitVertex();

            // Fourth triangle
            gl_Position = PV * pEndLeft;
            gl_Position.z -= u_depthOffset;
            cameraNormalToFrag = mat3(u_modelView) * Normal[2];
            colorToFrag = Color[2];
            intensityToFrag = 0.0;
            EmitVertex();

            EndPrimitive();
        }

)"
};



const ShaderStageSpecification RIBBON_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    {}, // uniforms
    {}, // attributes
    {}, // textures 
 
    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 colorToFrag;
        in vec3 cameraNormalToFrag;
        in float intensityToFrag;
        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$

        void main()
        {
           
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER }$

           // Compute a fade factor to set the transparency
           // Basically amounts to antialiasing in screen space when lines are relatively large on screen
           float screenFadeLen = 2.5;
           float dF = length(vec2(dFdx(intensityToFrag),dFdy(intensityToFrag)));
           float thresh = min(dF * screenFadeLen, 0.2);
           float fadeFactor = smoothstep(0, thresh, intensityToFrag);

           vec3 albedoColor = colorToFrag;
           vec3 shadeNormal = cameraNormalToFrag;
           
           // Lighting
           ${ GENERATE_LIT_COLOR }$
           
           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$
           alphaOut *= fadeFactor;

           // Write output
           outputF = vec4(litColor, alphaOut);
        }
)"
};

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

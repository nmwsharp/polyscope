// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader RIBBON_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_color", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
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
    )
};


static const GeomShader RIBBON_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_ribbonWidth", GLData::Float},
        {"u_depthOffset", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
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

    )
};



static const FragShader RIBBON_FRAG_SHADER = {
    
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
    },
    
    // output location
    "outputF",
 
    // source
    POLYSCOPE_GLSL(150,
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        in vec3 colorToFrag;
        in vec3 cameraNormalToFrag;
        in float intensityToFrag;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

        void main()
        {
           outputF = lightSurfaceMat(cameraNormalToFrag, colorToFrag, t_mat_r, t_mat_g, t_mat_b);

           // Compute a fade factor to set the transparency
           // Basically amounts to antialiasing in screen space when lines are relatively large on screen
           float screenFadeLen = 2.5;
           float dF = length(vec2(dFdx(intensityToFrag),dFdy(intensityToFrag)));
           float thresh = min(dF * screenFadeLen, 0.2);
           float fadeFactor = smoothstep(0, thresh, intensityToFrag);

           outputF.a = fadeFactor;
        }
    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

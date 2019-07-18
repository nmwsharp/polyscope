// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader SPHERE_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        void main()
        {
            gl_Position = vec4(a_position,1.0);
        }
    )
};

static const VertShader SPHERE_VALUE_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_value", GLData::Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        in float a_value;
        out float value;

        void main()
        {
            gl_Position = vec4(a_position,1.0);
            value = a_value;
        }
    )
};


static const VertShader SPHERE_COLOR_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_color", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        in vec3 a_color;
        out vec3 Color;
        void main()
        {
            Color = a_color;
            gl_Position = vec4(a_position,1.0);
        }
    )
};



static const GeomShader SPHERE_BILLBOARD_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        out vec3 worldPosToFrag;
        out vec2 boxCoord;
        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            { // Lower left
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                boxCoord = vec2(-1.,-1.);
                EmitVertex();
            }
            
            { // Lower right
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                boxCoord = vec2(1.,-1.);
                EmitVertex();
            }
            
            { // Upper left
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                boxCoord = vec2(-1.,1.);
                EmitVertex();
            }
            
            { // Upper right
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                boxCoord = vec2(1.,1.);
                EmitVertex();
            }
    
            EndPrimitive();

        }
    )
};

static const GeomShader SPHERE_VALUE_BILLBOARD_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        in float value[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        flat out float valueToFrag;
        out vec3 worldPosToFrag;
        out vec2 boxCoord;
        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            { // Lower left
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                valueToFrag = value[0];
                boxCoord = vec2(-1.,-1.);
                EmitVertex();
            }
            
            { // Lower right
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                valueToFrag = value[0];
                boxCoord = vec2(1.,-1.);
                EmitVertex();
            }
            
            { // Upper left
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                valueToFrag = value[0];
                boxCoord = vec2(-1.,1.);
                EmitVertex();
            }
            
            { // Upper right
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                valueToFrag = value[0];
                boxCoord = vec2(1.,1.);
                EmitVertex();
            }
    
            EndPrimitive();

        }
    )
};

static const GeomShader SPHERE_COLOR_BILLBOARD_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        in vec3 Color[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        flat out vec3 colorToFrag;
        out vec3 worldPosToFrag;
        out vec2 boxCoord;
        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            { // Lower left
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                colorToFrag = Color[0];
                boxCoord = vec2(-1.,-1.);
                EmitVertex();
            }
            
            { // Lower right
                vec4 worldPos = gl_in[0].gl_Position + vec4((-u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                colorToFrag = Color[0];
                boxCoord = vec2(1.,-1.);
                EmitVertex();
            }
            
            { // Upper left
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp - u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                colorToFrag = Color[0];
                boxCoord = vec2(-1.,1.);
                EmitVertex();
            }
            
            { // Upper right
                vec4 worldPos = gl_in[0].gl_Position + vec4((u_camUp + u_camRight) * u_pointRadius, 0.);
                gl_Position = PV * worldPos;
                worldPosToFrag = worldPos.xyz;
                colorToFrag = Color[0];
                boxCoord = vec2(1.,1.);
                EmitVertex();
            }
    
            EndPrimitive();

        }
    )
};




static const FragShader SPHERE_BILLBOARD_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},


        {"u_baseColor", GLData::Vector3Float},
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
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform vec3 u_baseColor;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);


        void main()
        {

           // Compute geometry on billboard
           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           vec3 worldN = (zC * u_camZ + boxCoord.x * u_camRight + boxCoord.y * u_camUp);

           // Lighting
           vec3 Normal = mat3(u_modelView) * worldN;
           outputF = lightSurfaceMat(Normal, u_baseColor, t_mat_r, t_mat_g, t_mat_b);

           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};

static const FragShader SPHERE_VALUE_BILLBOARD_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},

        {"u_rangeLow", GLData::Float},
        {"u_rangeHigh", GLData::Float},
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
    
    // output location
    "outputF",
 
    // source
    POLYSCOPE_GLSL(150,
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform float u_rangeLow;
        uniform float u_rangeHigh;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler1D t_colormap;
        flat in float valueToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
        
        vec3 surfaceColor() {
          float t = (valueToFrag - u_rangeLow) / (u_rangeHigh - u_rangeLow);
          t = clamp(t, 0.f, 1.f);
          return texture(t_colormap, t).rgb;
        }

        void main()
        {

           // Compute geometry on billboard
           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           vec3 worldN = (zC * u_camZ + boxCoord.x * u_camRight + boxCoord.y * u_camUp);

           // Lighting
           vec3 Normal = mat3(u_modelView) * worldN;
           outputF = lightSurfaceMat(Normal, surfaceColor(), t_mat_r, t_mat_g, t_mat_b);
           
           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};



static const FragShader SPHERE_COLOR_BILLBOARD_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
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
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        flat in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

        void main()
        {
  
           // Set geometry for billboard
           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           vec3 worldN = (zC * u_camZ + boxCoord.x * u_camRight + boxCoord.y * u_camUp);
           
           // Lighting
           vec3 Normal = mat3(u_modelView) * worldN;
           outputF = lightSurfaceMat(Normal, colorToFrag, t_mat_r, t_mat_g, t_mat_b);
           
           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};


static const FragShader SPHERE_COLOR_PLAIN_BILLBOARD_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
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
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        flat in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>

        void main()
        {

           // Set geometry for billboard
           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           
           outputF = vec4(colorToFrag, 1.0);
           
           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader PASSTHRU_CYLINDER_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;

        out vec3 position_tip;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
        }
    )
};

static const VertShader CYLINDER_VALUE_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
        {"a_value", GLData::Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in float a_value;

        out vec3 position_tip;
        out float edge_value;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
            edge_value = a_value;
        }
    )
};

static const VertShader CYLINDER_COLOR_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
        {"a_color", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color;

        out vec3 position_tip;
        out vec3 edge_color;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
            edge_color = a_color;
        }
    )
};

static const VertShader CYLINDER_BLEND_VALUE_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
        {"a_value_tail", GLData::Float},
        {"a_value_tip", GLData::Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in float a_value_tail;
        in float a_value_tip;

        out vec3 position_tip;
        out float value_tail;
        out float value_tip;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
            value_tail = a_value_tail;
            value_tip = a_value_tip;
        }
    )
};

static const VertShader CYLINDER_BLEND_COLOR_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
        {"a_color_tail", GLData::Vector3Float},
        {"a_color_tip", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color_tail;
        in vec3 a_color_tip;

        out vec3 position_tip;
        out vec3 color_tail;
        out vec3 color_tip;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
            color_tail = a_color_tail;
            color_tip = a_color_tip;
        }
    )
};

static const VertShader CYLINDER_PICK_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position_tail", GLData::Vector3Float},
        {"a_position_tip", GLData::Vector3Float},
        {"a_color_tail", GLData::Vector3Float},
        {"a_color_tip", GLData::Vector3Float},
        {"a_color_edge", GLData::Vector3Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color_tail;
        in vec3 a_color_tip;
        in vec3 a_color_edge;

        out vec3 position_tip;
        out vec3 color_tail;
        out vec3 color_tip;
        out vec3 color_edge;

        void main()
        {
            gl_Position = vec4(a_position_tail,1.0);
            position_tip = a_position_tip;
            color_tail = a_color_tail;
            color_tip = a_color_tip;
            color_edge = a_color_edge;
        }
    )
};



static const GeomShader CYLINDER_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=32) out;
        in vec3 position_tip[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 8;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};

static const GeomShader CYLINDER_VALUE_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=16) out;
        in vec3 position_tip[];
        in float edge_value[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out float value;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 4;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = edge_value[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = edge_value[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = edge_value[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = edge_value[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};

static const GeomShader CYLINDER_COLOR_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=16) out;
        in vec3 position_tip[];
        in vec3 edge_color[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 color;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 4;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = edge_color[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = edge_color[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = edge_color[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = edge_color[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};

static const GeomShader CYLINDER_BLEND_VALUE_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=16) out;
        in vec3 position_tip[];
        in float value_tip[];
        in float value_tail[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out float value;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 4;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = value_tail[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = value_tail[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = value_tip[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    value = value_tip[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};

static const GeomShader CYLINDER_BLEND_COLOR_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=16) out;
        in vec3 position_tip[];
        in vec3 color_tip[];
        in vec3 color_tail[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 color;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 4;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = color_tail[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = color_tail[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = color_tip[0];
                    cameraNormal = mat3(u_modelView) * norm0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    color = color_tip[0];
                    cameraNormal = mat3(u_modelView) * norm1;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};

static const GeomShader CYLINDER_PICK_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_modelView", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_radius", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=16) out;
        in vec3 position_tip[];
        in vec3 color_tip[];
        in vec3 color_tail[];
        in vec3 color_edge[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        flat out vec3 colorTail;
        flat out vec3 colorTip;
        flat out vec3 colorEdge;
        out float tEdge;

        // TODO make sure this isn't outputting too much data for common platforms

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 4;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = position_tip[0];
            vec3 shaft = capP - rootP;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(shaft, arbVec));
            vec3 radY = normalize(cross(shaft, radX));

            // Generate each panel around the vector
            for(int iTheta = 0; iTheta < nTheta; iTheta++) {

                float theta0 = delTheta * iTheta;
                float theta1 = delTheta * (iTheta+1);

                float x0 = cos(theta0);
                float y0 = sin(theta0);
                float x1 = cos(theta1);
                float y1 = sin(theta1);

                vec3 norm0 = (x0 * radX + y0 * radY);
                vec3 norm1 = (x1 * radX + y1 * radY);

                { // Lower left
                    vec4 worldPos = vec4(rootP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    colorTail = color_tail[0];
                    colorTip = color_tip[0];
                    colorEdge = color_edge[0];
                    tEdge = 0.0;
                    EmitVertex();
                }
                
                { // Lower right
                    vec4 worldPos = vec4(rootP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    colorTail = color_tail[0];
                    colorTip = color_tip[0];
                    colorEdge = color_edge[0];
                    tEdge = 0.0;
                    EmitVertex();
                }
                
                { // Upper left
                    vec4 worldPos = vec4(capP + norm0 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    colorTail = color_tail[0];
                    colorTip = color_tip[0];
                    colorEdge = color_edge[0];
                    tEdge = 1.0;
                    EmitVertex();
                }
                
                { // Upper right
                    vec4 worldPos = vec4(capP + norm1 * u_radius, 1.);
                    gl_Position = PV * worldPos;
                    colorTail = color_tail[0];
                    colorTip = color_tip[0];
                    colorEdge = color_edge[0];
                    tEdge = 1.0;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};



static const FragShader CYLINDER_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_color", GLData::Vector3Float},
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
        uniform vec3 u_color;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        in vec3 cameraNormal;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

        void main()
        {
           outputF = lightSurfaceMat(cameraNormal, u_color, t_mat_r, t_mat_g, t_mat_b);
        }
    )
};

static const FragShader CYLINDER_VALUE_FRAG_SHADER = {
    
    // uniforms
    {
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
        uniform float u_rangeLow;
        uniform float u_rangeHigh;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler1D t_colormap;
        in float value;
        in vec3 cameraNormal;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
        
        vec3 surfaceColor() {
          float t = (value - u_rangeLow) / (u_rangeHigh - u_rangeLow);
          t = clamp(t, 0.f, 1.f);
          return texture(t_colormap, t).rgb;
        }
        
        void main()
        {
            outputF = lightSurfaceMat(cameraNormal, surfaceColor(), t_mat_r, t_mat_g, t_mat_b);
        }
    )
};

static const FragShader CYLINDER_COLOR_FRAG_SHADER = {
    
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
        in vec3 color;
        in vec3 cameraNormal;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
        
        void main()
        {
            outputF = lightSurfaceMat(cameraNormal, color, t_mat_r, t_mat_g, t_mat_b);
        }
    )
};

static const FragShader CYLINDER_PICK_FRAG_SHADER = {
    
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
        flat in vec3 colorTail;
        flat in vec3 colorTip;
        flat in vec3 colorEdge;
        in float tEdge;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        
        void main()
        {

          float endWidth = 0.2; // size of buffer on the end where tip/tail color is used

          vec3 myColor;
          if(tEdge < endWidth) {
            myColor = colorTail;
          } else if (tEdge < (1.0f - endWidth)) {
            myColor = colorEdge;
          } else {
            myColor = colorTip;
          }

          outputF = vec4(myColor, 1.0f);
        }
    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

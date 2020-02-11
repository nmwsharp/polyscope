// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification SPHERE_VERT_SHADER = {

    ShaderStageType::Vertex,

    {}, // uniforms

    // attributes
    {
        {"a_position", DataType::Vector3Float},
    },
		
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        void main()
        {
            gl_Position = vec4(a_position,1.0);
        }
    )
};

const ShaderStageSpecification SPHERE_VALUE_VERT_SHADER = {
    
    ShaderStageType::Vertex,

    { }, // uniforms

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_value", DataType::Float},
    },
		
    {}, // textures

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


const ShaderStageSpecification SPHERE_COLOR_VERT_SHADER = {
    
    ShaderStageType::Vertex,

    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_color", DataType::Vector3Float},
    },

		{}, // textures

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



const ShaderStageSpecification SPHERE_BILLBOARD_GEOM_SHADER = {

    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
    }, 

    // attributes
    {
    },
		
    {}, // textures

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

const ShaderStageSpecification SPHERE_VALUE_BILLBOARD_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
    }, 

    // attributes
    {
    },
		
    {}, // textures

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

const ShaderStageSpecification SPHERE_COLOR_BILLBOARD_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
    }, 

    {}, // attributes
		
    {}, // textures

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




const ShaderStageSpecification SPHERE_BILLBOARD_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    { // uniforms
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},


        {"u_baseColor", DataType::Vector3Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
    
    // source
    POLYSCOPE_GLSL(330 core,
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
        uniform sampler2D t_mat_k;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        layout(location = 0) out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);


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
           outputF = vec4(lightSurfaceMat(Normal, u_baseColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};

const ShaderStageSpecification SPHERE_VALUE_BILLBOARD_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    { // uniforms
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    }, 

    { }, // attributes
    
    { // textures 
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
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
        uniform sampler2D t_mat_k;
        uniform sampler1D t_colormap;
        flat in float valueToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        layout(location = 0) out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        
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
           outputF = vec4(lightSurfaceMat(Normal, surfaceColor(), t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
           
           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};



const ShaderStageSpecification SPHERE_COLOR_BILLBOARD_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        flat in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        layout(location = 0) out vec4 outputF;

        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

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
           outputF = vec4(lightSurfaceMat(Normal, colorToFrag, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
           
           // Set depth (expensive!)
           vec3 zOffset = -zC * u_camZ * u_pointRadius;
           vec3 realWorldPos = worldPosToFrag + zOffset;
           vec4 clipPos = u_projMatrix * u_modelView * vec4(realWorldPos, 1.0);
           float ndcDepth = clipPos.z / clipPos.w;
           gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
        }
    )
};


const ShaderStageSpecification SPHERE_COLOR_PLAIN_BILLBOARD_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_camRight", DataType::Vector3Float},
        {"u_camUp", DataType::Vector3Float},
        {"u_camZ", DataType::Vector3Float},
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_pointRadius", DataType::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        flat in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        layout(location = 0) out vec4 outputF;

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

} // namespace render
} // namespace polyscope

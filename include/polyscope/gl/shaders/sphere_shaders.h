#pragma once


// NOTE: You probably don't want to include this directly... see shaders.h

static const VertShader PASSTHRU_SPHERE_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
    },

    // source
    GLSL(150,
        in vec3 a_position;
        void main()
        {
            gl_Position = vec4(a_position,1.0);
        }
    )
};

static const VertShader PASSTHRU_SPHERE_COLORED_VERT_SHADER = {
    // uniforms
    {
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_color", GLData::Vector3Float},
    },

    // source
    GLSL(150,
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



static const GeomShader SPHERE_GEOM_BILLBOARD_SHADER = {
    
    // uniforms
    {
        {"u_viewMatrix", GLData::Matrix44Float},
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
    GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=100) out;
        uniform mat4 u_viewMatrix;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        out vec3 worldPosToFrag;
        out vec2 boxCoord;
        void main()   {
            mat4 PV = u_projMatrix * u_viewMatrix;

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


static const GeomShader SPHERE_GEOM_COLORED_BILLBOARD_SHADER = {
    
    // uniforms
    {
        {"u_viewMatrix", GLData::Matrix44Float},
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
    GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=100) out;
        in vec3 Color[];
        uniform mat4 u_viewMatrix;
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        out vec3 colorToFrag;
        out vec3 worldPosToFrag;
        out vec2 boxCoord;
        void main()   {
            mat4 PV = u_projMatrix * u_viewMatrix;

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


static const FragShader SHINY_SPHERE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
        {"u_color", GLData::Vector3Float},
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
    GLSL(150,
        uniform vec3 u_eye;
        // uniform vec3 u_light;
        uniform vec3 u_lightCenter;
        uniform float u_lightDist;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        uniform vec3 u_color;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        // vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye );
        vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );

        void main()
        {

           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           vec3 worldN = (zC * u_camZ + boxCoord.x * u_camRight + boxCoord.y * u_camUp);

           outputF = lightSurface(worldPosToFrag, worldN, u_color, u_lightCenter, u_lightDist, u_eye);
        }
    )
};


static const FragShader SHINY_SPHERE_COLORED_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_lightCenter", GLData::Vector3Float},
        {"u_lightDist", GLData::Float},
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
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
    GLSL(150,
        uniform vec3 u_eye;
        // uniform vec3 u_light;
        uniform vec3 u_lightCenter;
        uniform float u_lightDist;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        // vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye );
        vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );

        void main()
        {

           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }
           float zC = sqrt(1.0 - r*r);
           vec3 worldN = (zC * u_camZ + boxCoord.x * u_camRight + boxCoord.y * u_camUp);

           outputF = lightSurface(worldPosToFrag, worldN, colorToFrag, u_lightCenter, u_lightDist, u_eye);
        }
    )
};


static const FragShader PLAIN_SPHERE_COLORED_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_camRight", GLData::Vector3Float},
        {"u_camUp", GLData::Vector3Float},
        {"u_camZ", GLData::Vector3Float},
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
    GLSL(150,
        uniform vec3 u_eye;
        // uniform vec3 u_light;
        uniform vec3 u_lightCenter;
        uniform float u_lightDist;
        uniform vec3 u_camRight;
        uniform vec3 u_camUp;
        uniform vec3 u_camZ;
        in vec3 colorToFrag;
        in vec3 worldPosToFrag;
        in vec2 boxCoord;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        // vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye );
        vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 lightC, float lightD, vec3 eye );

        void main()
        {

           float r = sqrt(boxCoord.x*boxCoord.x + boxCoord.y*boxCoord.y);
           if(r > 1.0) {
               discard;
           }

           outputF = vec4(colorToFrag, 1.0);
        }
    )
};



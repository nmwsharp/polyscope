#pragma once


// NOTE: You probably don't want to include this directly... see shaders.h


static const VertShader PASSTHRU_SPHERE_VERT_SHADER = {
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


static const GeomShader SPHERE_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_viewMatrix", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
        {"u_pointRadius", GLData::Float},
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
        out vec3 colorToFrag;
        out vec3 worldNormalToFrag;
        out vec3 worldPosToFrag;
        void main()   {
            mat4 PV = u_projMatrix * u_viewMatrix;
            const int nPhi =5;	// TODO does const actually work here?
            const int nTheta = 7;
            const float PI = 3.14159265358;
            const float delPhi = PI / (nPhi + 1);
            const float delTheta = 2*PI / nTheta;

            for (int iPhi = 0; iPhi <= nPhi; iPhi++) {

                float phiLower = - PI / 2.0 + (iPhi) * delPhi;
                float phiUpper = - PI / 2.0 + (iPhi+1) * delPhi;
                float cosPhiLower = cos(phiLower);
                float cosPhiUpper = cos(phiUpper);
                float zLower = sin(phiLower);
                float zUpper = sin(phiUpper);

                for (int iTheta = 0; iTheta <= nTheta; iTheta++) { /* duplicate first/last point to complete strip */

                    float theta = delTheta * iTheta;
                    float cosTheta = cos(theta);
                    float sinTheta = sin(theta);

                    // Lower point
                    float xLower = cosPhiLower * cosTheta;
                    float yLower = cosPhiLower * sinTheta;
                    float zLower = zLower;
                    vec4 worldPosLower = gl_in[0].gl_Position + vec4(xLower, yLower, zLower, 0) * u_pointRadius;
                    gl_Position = PV * worldPosLower;
                    worldPosToFrag = worldPosLower.xyz;
                    worldNormalToFrag = vec3(xLower, yLower, zLower);
                    colorToFrag = Color[0];
                    EmitVertex();

                    // Upper point
                    float xUpper = cosPhiUpper * cosTheta;
                    float yUpper = cosPhiUpper * sinTheta;
                    float zUpper = zUpper;
                    vec4 worldPosUpper = gl_in[0].gl_Position + vec4(xUpper, yUpper, zUpper, 0) * u_pointRadius;
                    gl_Position = PV * worldPosUpper;
                    worldPosToFrag = worldPosUpper.xyz;
                    worldNormalToFrag = vec3(xUpper, yUpper, zUpper);
                    colorToFrag = Color[0];
                    EmitVertex();

                }

                EndPrimitive();
            }

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


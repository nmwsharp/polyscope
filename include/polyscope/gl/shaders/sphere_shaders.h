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
        {"a_pointRadius", GLData::Float},
    },

    // source
    GLSL(150,
        in vec3 a_position;
        in vec3 a_color;
        in float a_pointRadius;
        out vec3 Color;
        out float pointRadius;
        void main()
        {
            Color = a_color;
            pointRadius = a_pointRadius;
            gl_Position = vec4(a_position,1.0);
        }
    )
};


static const GeomShader SPHERE_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_viewMatrix", GLData::Matrix44Float},
        {"u_projMatrix", GLData::Matrix44Float},
    }, 

    // attributes
    {
    },

    // source
    GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=100) out;
        in vec3 Color[];
        in float pointRadius[];
        uniform mat4 u_viewMatrix;
        uniform mat4 u_projMatrix;
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
                    vec4 worldPosLower = gl_in[0].gl_Position + vec4(xLower, yLower, zLower, 0) * pointRadius[0];
                    gl_Position = PV * worldPosLower;
                    worldPosToFrag = worldPosLower.xyz;
                    worldNormalToFrag = vec3(xLower, yLower, zLower);
                    colorToFrag = Color[0];
                    EmitVertex();

                    // Upper point
                    float xUpper = cosPhiUpper * cosTheta;
                    float yUpper = cosPhiUpper * sinTheta;
                    float zUpper = zUpper;
                    vec4 worldPosUpper = gl_in[0].gl_Position + vec4(xUpper, yUpper, zUpper, 0) * pointRadius[0];
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



static const FragShader SHINY_SPHERE_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_eye", GLData::Vector3Float},
        {"u_light", GLData::Vector3Float},
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
        uniform vec3 u_light;
        in vec3 colorToFrag;
        in vec3 worldNormalToFrag;
        in vec3 worldPosToFrag;
        out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye );

        void main()
        {
           outputF = lightSurface(worldPosToFrag, worldNormalToFrag, colorToFrag, u_light, u_eye );
        }
    )
};


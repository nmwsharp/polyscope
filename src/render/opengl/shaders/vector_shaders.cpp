// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification PASSTHRU_VECTOR_VERT_SHADER = {
  
		ShaderStageType::Vertex,

    { }, // uniforms

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_vector", DataType::Vector3Float},
    },
		
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        in vec3 a_vector;

        out vec3 vector;

        void main()
        {
            gl_Position = vec4(a_position,1.0);
            vector = a_vector;
        }
    )
};



const ShaderStageSpecification VECTOR_GEOM_SHADER = {
		
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_lengthMult", DataType::Float},
        {"u_radius", DataType::Float},
    }, 

    { }, // attributes
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=40) out;
        in vec3 vector[];
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_lengthMult;
        uniform float u_radius;
        out vec3 cameraNormal;

        void main()   {
            mat4 PV = u_projMatrix * u_modelView;

            const int nTheta = 8;
            const float PI = 3.14159;
            const float delTheta = 2.*PI / nTheta;            

            // Points along the central axis
            vec3 rootP = gl_in[0].gl_Position.xyz;
            vec3 capP = rootP + .8 * vector[0] * u_lengthMult;
            vec3 tipP = rootP + vector[0] * u_lengthMult;

            // Orthogonal basis
            const vec3 arbVec = vec3(0.129873, -.70892, .58972);
            vec3 radX = normalize(cross(vector[0], arbVec));
            vec3 radY = normalize(cross(vector[0], radX));

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
                
                { // Tip
                    vec3 tipNormal = normalize(norm0 + norm1);
                    vec4 worldPos = vec4(tipP, 1.);
                    gl_Position = PV * worldPos;
                    cameraNormal = mat3(u_modelView) * tipNormal;
                    EmitVertex();
                }
        
                EndPrimitive();

            }

        }
    )
};



const ShaderStageSpecification VECTOR_FRAG_SHADER = {
		
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_color", DataType::Vector3Float},
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
        uniform vec3 u_color;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        in vec3 cameraNormal;
        layout(location = 0) out vec4 outputF;

        // Forward declarations of methods from <shaders/common.h>
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);

        void main()
        {
           outputF = vec4(lightSurfaceMat(cameraNormal, u_color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);
        }
    )
};

// clang-format on

} // namespace render
} // namespace polyscope

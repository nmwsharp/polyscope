#include "polyscope/render/opengl/shaders/volume_mesh_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

const ShaderStageSpecification SLICE_TETS_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {},

    // attributes
    {
        {"a_point_1", DataType::Vector3Float},
        {"a_point_2", DataType::Vector3Float},
        {"a_point_3", DataType::Vector3Float},
        {"a_point_4", DataType::Vector3Float},
    },

    {}, // textures

    // source
    R"(
        ${ GLSL_VERSION }$

        in vec3 a_point_1;
        in vec3 a_point_2;
        in vec3 a_point_3;
        in vec3 a_point_4;
        out vec3 point_1;
        out vec3 point_2;
        out vec3 point_3;
        out vec3 point_4;
        
        void main()
        {
            point_1 = a_point_1;
            point_2 = a_point_2;
            point_3 = a_point_3;
            point_4 = a_point_4;
        }
)"};


const ShaderStageSpecification SLICE_TETS_GEOM_SHADER = {

    ShaderStageType::Geometry,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_slicePoint", DataType::Float},
        {"u_sliceNormal", DataType::Vector3Float},
    },

    // attributes
    {},

    {}, // textures

    // source
    R"(
        ${ GLSL_VERSION }$

        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        uniform float u_slicePoint;
        uniform vec3 u_sliceNormal;
        in vec3 point_1[];
        in vec3 point_2[];
        in vec3 point_3[];
        in vec3 point_4[];

        void main() {

            vec3 p[4] = vec3[](point_1[0], point_2[0], point_3[0], point_4[0]);
            
            float d[4]; 
            for (int i = 0; i < 4; i++ ) d[i] = dot(u_sliceNormal, p[i]) - u_slicePoint;

            vec3 q[4];
            int n = 0;
            for( int i = 0; i < 4; i++ ) {
                for( int j = i+1; j < 4; j++ ) {
                    if( d[i]*d[j] < 0. ) {
                        float t = (0-d[i])/(d[j]-d[i]);
                        q[n] = ( (1.-t)*p[i] + t*p[j] );
                        n++;
                    }
                }
            }

            if(n == 4){
                vec3 cross13 = cross(q[1] - q[0], q[3] - q[0]);
                vec3 cross23 = cross(q[2] - q[0], q[3] - q[0]);
                if(dot(cross13, cross23) > 0){
                    if(dot(cross23, cross23) < dot(cross13, cross13)){
                        vec3 temp = q[2];
                        q[2] = q[3];
                        q[3] = temp;
                    }else{
                        vec3 temp = q[1];
                        q[1] = q[3];
                        q[3] = temp;
                    }
                }
            }

            vec3 cross12 = cross(q[1] - q[0], q[2] - q[0]);
            if(dot(cross12, u_sliceNormal) < 0){
                vec3 temp = q[1];
                q[1] = q[2];
                q[2] = temp;
            }

            // Emit the vertices as a triangle strip
            mat4 toScreen = u_projMatrix * u_modelView;
            for (int i = 0; i < n; i++){
                gl_Position = toScreen * vec4(q[i], 1.0); 
                EmitVertex();
            }
            EndPrimitive();
        
        }

)"};

const ShaderStageSpecification SLICE_TETS_FRAG_SHADER = {

    ShaderStageType::Fragment,

    // uniforms
    {},

    {}, // attributes

    // textures
    {},

    // source
    R"(
        ${ GLSL_VERSION }$
        layout(location = 0) out vec4 outputF;

        void main()
        {
           outputF = vec4(1, 0, 0, 1);
        }
)"};

} // namespace backend_openGL3_glfw
} // namespace render
}; // namespace polyscope
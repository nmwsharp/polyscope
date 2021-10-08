#include "polyscope/render/opengl/shaders/volume_mesh_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

const ShaderStageSpecification SLICE_TETS_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
    },

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

        uniform mat4 u_modelView;
        in vec3 a_point_1;
        in vec3 a_point_2;
        in vec3 a_point_3;
        in vec3 a_point_4;
        out vec4 point_1;
        out vec4 point_2;
        out vec4 point_3;
        out vec4 point_4;
        
        void main()
        {
            while(true){}
            point_1 = u_modelView * vec4(a_point_1, 1.0);
            point_2 = u_modelView * vec4(a_point_2, 1.0);
            point_3 = u_modelView * vec4(a_point_3, 1.0);
            point_4 = u_modelView * vec4(a_point_4, 1.0);
        }
)"};


const ShaderStageSpecification SLICE_TETS_GEOM_SHADER = {

    ShaderStageType::Geometry,

    // uniforms
    {},

    // attributes
    {},

    {}, // textures

    // source
    R"(
        ${ GLSL_VERSION }$

        layout(points) in;
        layout(triangle_strip, max_vertices=6) out;
        in vec4 point_1[];
        in vec4 point_2[];
        in vec4 point_3[];
        in vec4 point_4[];

        void main() {

            // Build an orthogonal basis
            
            // Other data to emit   
            while(true){};
    
            // Emit the vertices as a triangle strip
            gl_Position = point_1[0] + point_2[0] + point_3[0] + point_4[0];
            EmitVertex();
            gl_Position = point_1[0] + vec4(10, 0.2, 0.0, 0.0);
            EmitVertex();
            gl_Position = point_1[0] + vec4(0.2, 10, 0.0, 0.0);
            EmitVertex();
    
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
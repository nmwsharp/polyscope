// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
        {"a_point_1", RenderDataType::Vector3Float},
        {"a_slice_1", RenderDataType::Vector3Float},
        {"a_point_2", RenderDataType::Vector3Float},
        {"a_slice_2", RenderDataType::Vector3Float},
        {"a_point_3", RenderDataType::Vector3Float},
        {"a_slice_3", RenderDataType::Vector3Float},
        {"a_point_4", RenderDataType::Vector3Float},
        {"a_slice_4", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
    R"(
        ${ GLSL_VERSION }$
        ${ VERT_DECLARATIONS }$

        in vec3 a_point_1;
        in vec3 a_point_2;
        in vec3 a_point_3;
        in vec3 a_point_4;
        in vec3 a_slice_1;
        in vec3 a_slice_2;
        in vec3 a_slice_3;
        in vec3 a_slice_4;
        out vec3 point_1;
        out vec3 point_2;
        out vec3 point_3;
        out vec3 point_4;
        out vec3 slice_1;
        out vec3 slice_2;
        out vec3 slice_3;
        out vec3 slice_4;

        void main()
        {
            point_1 = a_point_1;
            point_2 = a_point_2;
            point_3 = a_point_3;
            point_4 = a_point_4;
            slice_1 = a_slice_1;
            slice_2 = a_slice_2;
            slice_3 = a_slice_3;
            slice_4 = a_slice_4;
            ${ VERT_ASSIGNMENTS }$
        }
)"};


const ShaderStageSpecification SLICE_TETS_GEOM_SHADER = {

    ShaderStageType::Geometry,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_slicePoint", RenderDataType::Float},
        {"u_sliceVector", RenderDataType::Vector3Float},
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
        uniform vec3 u_sliceVector;
        in vec3 point_1[];
        in vec3 point_2[];
        in vec3 point_3[];
        in vec3 point_4[];
        in vec3 slice_1[];
        in vec3 slice_2[];
        in vec3 slice_3[];
        in vec3 slice_4[];
        out vec3 a_barycoordToFrag;
        out vec3 a_normalToFrag;
        ${ GEOM_DECLARATIONS }$

        void main() {

            vec3 s[4] = vec3[](slice_1[0], slice_2[0], slice_3[0], slice_4[0]);
            vec3 p[4] = vec3[](point_1[0], point_2[0], point_3[0], point_4[0]);
            ${ GEOM_INIT_DECLARATIONS }$
            int ordering[4] = int[](0, 1, 2, 3);
            
            float d[4]; 
            for (int i = 0; i < 4; i++ ) d[i] = dot(u_sliceVector, s[i]) - u_slicePoint;

            vec3 q[4];
            int n = 0;
            for( int i = 0; i < 4; i++ ) {
                for( int j = i+1; j < 4; j++ ) {
                    if( d[i]*d[j] < 0. ) {
                        float t = (0-d[i])/(d[j]-d[i]);
                        ${ GEOM_INTERPOLATE }$
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
                        ordering[2] = 3;
                        ordering[3] = 2;
                    }else{
                        ordering[1] = 3;
                        ordering[3] = 1;
                    }
                }
            }

            // compute gradient of d to orient our sliced faces
            vec3 vN[4]; // tet face normal times volume
            vN[0] = cross(p[2]-p[1], p[3]-p[1]);
            vN[1] = cross(p[3]-p[0], p[2]-p[0]);
            vN[2] = cross(p[1]-p[0], p[3]-p[0]);
            vN[3] = cross(p[0]-p[1], p[2]-p[1]);

            // actually gradient times tet volume, but since we only want the sign, that doesn't really matter
            // TODO: fix for inverted tets?
            vec3 grad = d[0] * vN[0] + d[1] * vN[1] + d[2] * vN[2] + d[3] * vN[3];
            vec3 cross12 = cross(q[1] - q[0], q[2] - q[0]);
            if(dot(cross12, grad) < 0){
                int temp = ordering[1];
                ordering[1] = ordering[2];
                ordering[2] = temp;
                cross12 *= -1;
            }
            // Offset slice so that tet edges don't clip through
            vec3 offset = u_sliceVector * 1e-4;
            // Emit the vertices as a triangle strip
            mat4 toScreen = u_projMatrix * u_modelView;
            for (int i = 0; i < n; i++){
                a_normalToFrag = vec3(toScreen * vec4(cross12, 0.0));
                a_barycoordToFrag = vec3(0, 0, 0);
                a_barycoordToFrag[i % 3] = 1.0;
                ${ GEOM_ASSIGNMENTS }$
                gl_Position = toScreen * vec4(q[ordering[i]] - offset, 1.0); 
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
        in vec3 a_normalToFrag;
        in vec3 a_barycoordToFrag;
        layout(location = 0) out vec4 outputF;

        ${ FRAG_DECLARATIONS }$
        ${ SLICE_TETS_FRAG_DECLARATIONS }$ 

        void main()
        {
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$
        
          
           // Shading
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$
           
           // Handle the wireframe
           ${ APPLY_WIREFRAME }$

           // Lighting
           vec3 shadeNormal = a_normalToFrag;
           ${ PERTURB_SHADE_NORMAL }$
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$
           
           // silly dummy usage to ensure normal and barycoords are always used; otherwise we get errors
           float dummyVal = a_normalToFrag.x + a_barycoordToFrag.x;
           alphaOut = alphaOut + dummyVal * (1e-12);

           ${ PERTURB_LIT_COLOR }$

           // Write output
           outputF = vec4(litColor, alphaOut);
        }
)"};

const ShaderReplacementRule SLICE_TETS_BASECOLOR_SHADE(
    /* rule name */ "SLICE_TETS_BASECOLOR_SHADE",
    {/* replacement sources */
     {"FRAG_DECLARATIONS", R"(
          uniform vec3 u_baseColor1;
          in float a_faceColorTypeToFrag;
        )"},
     {"GENERATE_SHADE_COLOR", R"(
          vec3 albedoColor = u_baseColor1;
        )"}},
    /* uniforms */
    {
        {"u_baseColor1", RenderDataType::Vector3Float},
    },
    /* attributes */ {},
    /* textures */ {});

const ShaderReplacementRule SLICE_TETS_MESH_WIREFRAME(
    /* rule name */ "SLICE_TETS_MESH_WIREFRAME",
    {
        /* replacement sources */
        {"GEOM_DECLARATIONS", R"(
          out vec3 a_edgeIsRealToFrag;
        )"},
        {"GEOM_ASSIGNMENTS", R"(
          vec3 edgeRealV = vec3(1, 1, 1);
          a_edgeIsRealToFrag = edgeRealV;
        )"},
        {"FRAG_DECLARATIONS", R"(
          in vec3 a_edgeIsRealToFrag;

          uniform float u_edgeWidth;
          uniform vec3 u_edgeColor;
      
          float getEdgeFactor(vec3 UVW, vec3 edgeReal, float width) {
            // The Nick Sharp Edge Function. There are many like it, but this one is mine.
            float slopeWidth = 1.;
            
            vec3 fw = fwidth(UVW);
            vec3 realUVW = max(UVW, 1.0 - edgeReal.yzx);
            vec3 baryWidth = slopeWidth * fw;

            vec3 end = width*fw;
            vec3 dist = smoothstep(end - baryWidth, end, realUVW);

            float e = 1.0 - min(min(dist.x, dist.y), dist.z);
            return e;
          }
        )"},
        {"APPLY_WIREFRAME", R"(
          float edgeFactor = getEdgeFactor(a_barycoordToFrag, a_edgeIsRealToFrag, u_edgeWidth);
          albedoColor = mix(albedoColor, u_edgeColor, edgeFactor);
      )"},
    },
    /* uniforms */
    {
        {"u_edgeColor", RenderDataType::Vector3Float},
        {"u_edgeWidth", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {});
const ShaderReplacementRule SLICE_TETS_PROPAGATE_VECTOR(
    /* rule name */ "SLICE_TETS_PROPAGATE_VECTOR",
    {
        /* replacement sources */
        {"VERT_DECLARATIONS", R"(
          in vec3 a_value_1;
          in vec3 a_value_2;
          in vec3 a_value_3;
          in vec3 a_value_4;
          out vec3 value_1;
          out vec3 value_2;
          out vec3 value_3;
          out vec3 value_4;
        )"},
        {"VERT_ASSIGNMENTS", R"(
          value_1 = a_value_1;
          value_2 = a_value_2;
          value_3 = a_value_3;
          value_4 = a_value_4;
        )"},
        {"GEOM_DECLARATIONS", R"(
          in vec3 value_1[];
          in vec3 value_2[];
          in vec3 value_3[];
          in vec3 value_4[];
          out vec3 a_valueToFrag;
      )"},
        {"GEOM_INIT_DECLARATIONS", R"(
          vec3 v[4] = vec3[](value_1[0], value_2[0], value_3[0], value_4[0]);
          vec3 out_v[4] = vec3[](vec3(0), vec3(0), vec3(0), vec3(0));
      )"},
        {"GEOM_INTERPOLATE", R"(
          out_v[n] = ( (1.-t)*v[i] + t*v[j] );
      )"},
        {"GEOM_ASSIGNMENTS", R"(
          a_valueToFrag = out_v[ordering[i]];
      )"},
        {"FRAG_DECLARATIONS", R"(
          in vec3 a_valueToFrag;
        )"},
        {"GENERATE_SHADE_VALUE", R"(
          vec3 shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */
    {
        {"a_value_1", RenderDataType::Vector3Float},
        {"a_value_2", RenderDataType::Vector3Float},
        {"a_value_3", RenderDataType::Vector3Float},
        {"a_value_4", RenderDataType::Vector3Float},
    },
    /* textures */ {});

const ShaderReplacementRule SLICE_TETS_VECTOR_COLOR(
    /* rule name */ "SLICE_TETS_VECTOR_COLOR",
    {/* replacement sources */
     {"GENERATE_SHADE_COLOR", R"(
          vec3 albedoColor = shadeValue;
        )"}},
    /* uniforms */
    {},
    /* attributes */ {},
    /* textures */ {});

const ShaderReplacementRule SLICE_TETS_PROPAGATE_VALUE(
    /* rule name */ "SLICE_TETS_PROPAGATE_VALUE",
    {
        /* replacement sources */
        {"VERT_DECLARATIONS", R"(
          in float a_value_1;
          in float a_value_2;
          in float a_value_3;
          in float a_value_4;
          out float value_1;
          out float value_2;
          out float value_3;
          out float value_4;
        )"},
        {"VERT_ASSIGNMENTS", R"(
          value_1 = a_value_1;
          value_2 = a_value_2;
          value_3 = a_value_3;
          value_4 = a_value_4;
        )"},
        {"GEOM_DECLARATIONS", R"(
          in float value_1[];
          in float value_2[];
          in float value_3[];
          in float value_4[];
          out float a_valueToFrag;
      )"},
        {"GEOM_INIT_DECLARATIONS", R"(
          float v[4] = float[](value_1[0], value_2[0], value_3[0], value_4[0]);
          float out_v[4] = float[](0, 0, 0, 0);
      )"},
        {"GEOM_INTERPOLATE", R"(
          out_v[n] = ( (1.-t)*v[i] + t*v[j] );
      )"},
        {"GEOM_ASSIGNMENTS", R"(
          a_valueToFrag = out_v[ordering[i]];
      )"},
        {"FRAG_DECLARATIONS", R"(
          in float a_valueToFrag;
        )"},
        {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */
    {
        {"a_value_1", RenderDataType::Float},
        {"a_value_2", RenderDataType::Float},
        {"a_value_3", RenderDataType::Float},
        {"a_value_4", RenderDataType::Float},
    },
    /* textures */ {});

} // namespace backend_openGL3_glfw
} // namespace render
}; // namespace polyscope

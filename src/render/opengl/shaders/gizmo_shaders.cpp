#include "polyscope/render/opengl/shaders/gizmo_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_VERT = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
        {"u_projMatrix", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_normal", DataType::Vector3Float},
        {"a_color", DataType::Vector3Float},
        {"a_texcoord", DataType::Vector2Float},
        {"a_component", DataType::Vector3Float}, // holds 1 for which axis it is
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;
        in vec3 a_position;
        in vec3 a_normal;
        in vec3 a_color;
        in vec3 a_component;
        in vec2 a_texcoord;
        out vec3 a_componentToFrag;
        out vec3 a_normalToFrag;
        out vec3 a_colorToFrag;
        out vec2 a_texcoordToFrag;
        
        void main()
        {
            gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
            a_normalToFrag = mat3(u_modelView) * a_normal;
            a_colorToFrag = a_color;
            a_componentToFrag = a_component;
            a_texcoordToFrag = a_texcoord;
        }
)"
};

const ShaderStageSpecification TRANSFORMATION_GIZMO_ROT_FRAG = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_diskWidthRel", DataType::Float},
        {"u_active", DataType::Vector3Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$
        in vec3 a_normalToFrag;
        in vec3 a_colorToFrag;
        in vec3 a_componentToFrag;
        in vec2 a_texcoordToFrag;
        uniform float u_diskWidthRel;
        uniform vec3 u_active;
        layout(location = 0) out vec4 outputF;

        void main()
        {
           float depth = gl_FragCoord.z;
          
           // Set alpha
           float diskWidth = u_diskWidthRel;
           float diskRad = 1. - diskWidth;
           float pointRad = length(a_texcoordToFrag);
           float distFromRing = abs(pointRad - diskRad);
           float ringFactor = distFromRing / diskWidth;
           //float fw = fwidth(ringFactor);
           //float shadeFactor = smoothstep(end - baryWidth, end, realUVW);
           float shadeFactor = 1.;
           if(ringFactor > 1.) {
             shadeFactor = 0.;
           }

           float alphaOut = shadeFactor;
           if(alphaOut == 0.) discard;

           // Set the color
           vec3 albedoColor = a_colorToFrag;

           vec3 activeMask = a_componentToFrag * u_active;
           bool isActive = (activeMask.x + activeMask.y +activeMask.z) != 0.;
           if(isActive) {
             albedoColor = mix(albedoColor, vec3(1., 1., 1.), 0.3);
           }
           
           // Lighting
           vec3 shadeNormal = a_normalToFrag;

           // Write output
           outputF = vec4(albedoColor, alphaOut);
           //outputF = vec4(albedoColor, 1.);
           //outputF = vec4(1., 1., 0., 1.);
        }
)"
};

const ShaderReplacementRule TRANSFORMATION_GIZMO_VEC (
    /* rule name */ "TRANSFORMATION_GIZMO_VEC",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_component;
          out vec3 a_componentToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_componentToGeom = a_component;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_componentToGeom[];
          out vec3 a_componentToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_componentToFrag = a_componentToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_componentToFrag;
          uniform vec3 u_active;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
         vec3 activeMask = a_componentToFrag * u_active;
         bool isActive = (activeMask.x + activeMask.y +activeMask.z) != 0.;
         if(isActive) {
           shadeColor = mix(shadeColor, vec3(1., 1., 1.), 0.3);
         }
        )"},
    },
    /* uniforms */ {
      {"u_active", DataType::Vector3Float},
    },
    /* attributes */ {
      {"a_component", DataType::Vector3Float}
    },
    /* textures */ {}
);

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

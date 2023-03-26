// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/render/opengl/shaders/sphere_shaders.h"


namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification FLEX_SPHERE_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_position;
        uniform mat4 u_modelView;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_modelView * vec4(a_position, 1.0);

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_SPHERE_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_pointRadius", RenderDataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        in vec4 position_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;
        out vec3 sphereCenterView;

        ${ GEOM_DECLARATIONS }$

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {
           
            float pointRadius = u_pointRadius;
            ${ SPHERE_SET_POINT_RADIUS_GEOM }$
            
            // Construct the 4 corners of a billboard quad, facing the camera
            // Quad is shifted pointRadius toward the camera, otherwise it doesn't actually necessarily
            // cover the full sphere due to perspective.
            vec3 dirToCam = normalize(-gl_in[0].gl_Position.xyz);
            vec3 basisX;
            vec3 basisY;
            buildTangentBasis(dirToCam, basisX, basisY);
            vec4 center = u_projMatrix * (gl_in[0].gl_Position + vec4(dirToCam, 0.) * pointRadius);
            vec4 dx = u_projMatrix * (vec4(basisX, 0.) * pointRadius);
            vec4 dy = u_projMatrix * (vec4(basisY, 0.) * pointRadius);
            vec4 p1 = center - dx - dy;
            vec4 p2 = center + dx - dy;
            vec4 p3 = center - dx + dy;
            vec4 p4 = center + dx + dy;
            
            // Other data to emit   
            ${ GEOM_COMPUTE_BEFORE_EMIT }$
            vec3 sphereCenterViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
    
            // Emit the vertices as a triangle strip
            ${ GEOM_PER_EMIT }$ sphereCenterView = sphereCenterViewVal; gl_Position = p1; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ sphereCenterView = sphereCenterViewVal; gl_Position = p2; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ sphereCenterView = sphereCenterViewVal; gl_Position = p3; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ sphereCenterView = sphereCenterViewVal; gl_Position = p4; EmitVertex(); 
    
            EndPrimitive();

        }

)"
};

const ShaderStageSpecification FLEX_SPHERE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_invProjMatrix", RenderDataType::Matrix44Float},
        {"u_viewport", RenderDataType::Vector4Float},
        {"u_pointRadius", RenderDataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$
        uniform mat4 u_projMatrix; 
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_pointRadius;
        in vec3 sphereCenterView;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool raySphereIntersection(vec3 rayStart, vec3 rayDir, vec3 sphereCenter, float sphereRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        
        ${ FRAG_DECLARATIONS }$

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           float pointRadius = u_pointRadius;
           ${ SPHERE_SET_POINT_RADIUS_FRAG }$

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           bool hit = raySphereIntersection(vec3(0., 0., 0), viewRay, sphereCenterView, pointRadius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);

           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$
           
           // Set depth (expensive!)
           gl_FragDepth = depth;
          
           // Shading
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$

           // Lighting
           vec3 shadeNormal = nHit;
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$

           // Write output
           outputF = vec4(litColor, alphaOut);
        }
)"
};

//  These POINTQUAD shaders render a quad at the location of the point. Technically, 
//  they don't draw spheres, but we group them here because they share a lot of logic 
//  with the spheres & accept the same rules.

const ShaderStageSpecification FLEX_POINTQUAD_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_position;
        uniform mat4 u_modelView;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_modelView * vec4(a_position, 1.0);

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_POINTQUAD_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_pointRadius", RenderDataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        layout(points) in;
        layout(triangle_strip, max_vertices=4) out;
        in vec4 position_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_pointRadius;

        ${ GEOM_DECLARATIONS }$

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {
           
            float pointRadius = u_pointRadius;
            ${ SPHERE_SET_POINT_RADIUS_GEOM }$
            
            // Construct the 4 corners of a billboard quad, facing the camera
            vec3 dirToCam = normalize(-gl_in[0].gl_Position.xyz);
            vec3 basisX;
            vec3 basisY;
            buildTangentBasis(dirToCam, basisX, basisY);
            vec4 center = u_projMatrix * gl_in[0].gl_Position;
            vec4 dx = u_projMatrix * (vec4(basisX, 0.) * pointRadius);
            vec4 dy = u_projMatrix * (vec4(basisY, 0.) * pointRadius);
            vec4 p1 = center - dx - dy;
            vec4 p2 = center + dx - dy;
            vec4 p3 = center - dx + dy;
            vec4 p4 = center + dx + dy;
            
            ${ GEOM_COMPUTE_BEFORE_EMIT }$
    
            // Emit the vertices as a triangle strip
            ${ GEOM_PER_EMIT }$ gl_Position = p1; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ gl_Position = p2; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ gl_Position = p3; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ gl_Position = p4; EmitVertex(); 
    
            EndPrimitive();

        }

)"
};


const ShaderStageSpecification FLEX_POINTQUAD_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_pointRadius", RenderDataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
R"(
        ${ GLSL_VERSION }$
        uniform mat4 u_projMatrix; 
        uniform float u_pointRadius;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        
        ${ FRAG_DECLARATIONS }$

        void main()
        {
           
           float depth = gl_FragCoord.z;
           ${ GLOBAL_FRAGMENT_FILTER_PREP }$
           ${ GLOBAL_FRAGMENT_FILTER }$

           // TODO (?) make it a disk rather than a quad by clipping points outside
           // the radius.
           float pointRadius = u_pointRadius;
           ${ SPHERE_SET_POINT_RADIUS_FRAG }$
          
           // Shading
           ${ GENERATE_SHADE_VALUE }$
           ${ GENERATE_SHADE_COLOR }$

           // Lighting
           vec3 shadeNormal = vec3(0.0, 0.0, 1.0); // use a constant normal pointing towards the camera
           ${ GENERATE_LIT_COLOR }$

           // Set alpha
           float alphaOut = 1.0;
           ${ GENERATE_ALPHA }$

           // Write output
           outputF = vec4(litColor, alphaOut);
        }
)"
};


// == Rules

const ShaderReplacementRule SPHERE_PROPAGATE_VALUE (
    /* rule name */ "SPHERE_PROPAGATE_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value;
          out float a_valueToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueToGeom = a_value;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_valueToGeom[];
          out float a_valueToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_valueToFrag = a_valueToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float shadeValue = a_valueToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule SPHERE_PROPAGATE_VALUE2 (
    /* rule name */ "SPHERE_PROPAGATE_VALUE2",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec2 a_value2;
          out vec2 a_value2ToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_value2ToGeom = a_value2;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec2 a_value2ToGeom[];
          out vec2 a_value2ToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_value2ToFrag = a_value2ToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec2 a_value2ToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec2 shadeValue2 = a_value2ToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value2", RenderDataType::Vector2Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule SPHERE_PROPAGATE_COLOR (
    /* rule name */ "SPHERE_PROPAGATE_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color;
          out vec3 a_colorToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorToGeom = a_color;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorToGeom[];
          flat out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorToFrag = a_colorToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 a_colorToFrag;
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          vec3 shadeColor = a_colorToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule SPHERE_CULLPOS_FROM_CENTER(
    /* rule name */ "SPHERE_CULLPOS_FROM_CENTER",
    { /* replacement sources */
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          vec3 cullPos = sphereCenterView;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

// use a separate version for the quads because they don't already pass the center, and we 
// don't want to always pass it
const ShaderReplacementRule SPHERE_CULLPOS_FROM_CENTER_QUAD(
    /* rule name */ "SPHERE_CULLPOS_FROM_CENTER_QUAD",
    { /* replacement sources */
      {"GEOM_DECLARATIONS", R"(
          out vec3 sphereCenterView;
        )"},
      {"GEOM_COMPUTE_BEFORE_EMIT", R"(
          vec3 sphereCenterViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
        )"},
      {"GEOM_PER_EMIT", R"(
          sphereCenterView = sphereCenterViewVal;
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 sphereCenterView;
        )"},
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          vec3 cullPos = sphereCenterView;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule SPHERE_VARIABLE_SIZE (
    /* rule name */ "SPHERE_VARIABLE_SIZE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_pointRadius;
          out float a_pointRadiusToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_pointRadiusToGeom = a_pointRadius;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_pointRadiusToGeom[];
          out float a_pointRadiusToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_pointRadiusToFrag = a_pointRadiusToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_pointRadiusToFrag;
        )"},
      {"SPHERE_SET_POINT_RADIUS_GEOM", R"(
          pointRadius *= a_pointRadiusToGeom[0];
        )"},
      {"SPHERE_SET_POINT_RADIUS_FRAG", R"(
          pointRadius *= a_pointRadiusToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_pointRadius", RenderDataType::Float},
    },
    /* textures */ {}
);

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

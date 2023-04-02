// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/vector_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification FLEX_VECTOR_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
        {"a_vector", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_position;
        in vec3 a_vector;
        uniform mat4 u_modelView;
        out vec4 vector;
        
        ${ VERT_DECLARATIONS }$
        

        void main()
        {
            gl_Position = u_modelView * vec4(a_position,1.0);
            vector = u_modelView * vec4(a_vector, 0.0);
            
            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_TANGENT_VECTOR_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
        {"u_vectorRotRad", RenderDataType::Float},
    }, 

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
        {"a_tangentVector", RenderDataType::Vector2Float},
        {"a_basisVectorX", RenderDataType::Vector3Float},
        {"a_basisVectorY", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_position;
        in vec2 a_tangentVector;
        in vec3 a_basisVectorX;
        in vec3 a_basisVectorY;
        uniform mat4 u_modelView;
        uniform float u_vectorRotRad;
        out vec4 vector;
        
        ${ VERT_DECLARATIONS }$
        

        void main()
        {
            gl_Position = u_modelView * vec4(a_position,1.0);
          
            vec2 rotTangentVector = a_tangentVector;
            if(u_vectorRotRad != 0.) {
              float cR = cos(u_vectorRotRad);
              float sR = sin(u_vectorRotRad);
              mat2 rotMat = mat2(cR, sR, -sR, cR);
              rotTangentVector = rotMat * rotTangentVector;
            }

            vec3 worldVector = rotTangentVector.x * a_basisVectorX + rotTangentVector.y * a_basisVectorY;
            vector = u_modelView * vec4(worldVector, .0);
            
            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_VECTOR_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_lengthMult", RenderDataType::Float},
        {"u_radius", RenderDataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$
        
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 vector[];
        uniform mat4 u_projMatrix;
        uniform float u_lengthMult;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;

        ${ GEOM_DECLARATIONS }$

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 vecViewVal = vector[0].xyz;
            vec3 tipViewVal = tailViewVal + vecViewVal * u_lengthMult;
            vec3 vecDir = normalize(vecViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(vecDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * vec4(tailViewVal, 1.0);
            vec4 tipProj = u_projMatrix * vec4(tipViewVal, 1.0);
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            ${ GEOM_PER_EMIT }$ tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

)"
};


const ShaderStageSpecification FLEX_VECTOR_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_invProjMatrix", RenderDataType::Matrix44Float},
        {"u_viewport", RenderDataType::Vector4Float},
        {"u_radius", RenderDataType::Float},
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
        uniform float u_radius;
        in vec3 tailView;
        in vec3 tipView;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        bool rayConeIntersection(vec3 rayStart, vec3 rayDir, vec3 coneBase, vec3 coneTip, float coneRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        
        ${ FRAG_DECLARATIONS }$

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);
           
           // geometric shape of hte vector
           float tipLengthFrac = 0.2;
           float tipWidthFrac = 0.6;
           float adjRadius = min(u_radius, length(tipView - tailView)*tipLengthFrac); // clip vector aspect ratio by shrinking width of small vectors (length is always an accurate representation of data)

           // Raycast to the cylinder 
           float tHit = LARGE_FLOAT();
           vec3 pHit = vec3(777,777,777);
           vec3 nHit =  vec3(777,777,777);
           vec3 cylEnd = tailView + (1. - tipLengthFrac) * (tipView - tailView);
           float cylRad = tipWidthFrac * adjRadius;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, cylEnd, cylRad, tHit, pHit, nHit);
           
           // Raycast to cone
           float tHitCone;
           vec3 pHitCone;
           vec3 nHitCone;
           bool coneHit = rayConeIntersection(vec3(0., 0., 0), viewRay, cylEnd, tipView, adjRadius, tHitCone, pHitCone, nHitCone);
           if(tHitCone < tHit) {
             tHit = tHitCone;
             pHit = pHitCone;
             nHit = nHitCone;
           }
        
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


// == Rules

const ShaderReplacementRule VECTOR_PROPAGATE_COLOR (
    /* rule name */ "VECTOR_PROPAGATE_COLOR",
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
          out vec3 a_colorToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorToFrag = a_colorToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_colorToFrag;
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

const ShaderReplacementRule VECTOR_CULLPOS_FROM_TAIL(
    /* rule name */ "VECTOR_CULLPOS_FROM_TAIL",
    { /* replacement sources */
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          vec3 cullPos = tailView;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);


// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

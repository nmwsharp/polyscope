// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification PASSTHRU_VECTOR_VERT_SHADER = {
  
    ShaderStageType::Vertex,

    { 
        {"u_modelView", DataType::Matrix44Float},
    }, // uniforms

    // attributes
    {
        {"a_position", DataType::Vector3Float},
        {"a_vector", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        uniform mat4 u_modelView;
        in vec3 a_position;
        in vec3 a_vector;

        out vec4 vector;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position,1.0);
            vector = u_modelView * vec4(a_vector, 0.0);
        }
    )
};



const ShaderStageSpecification VECTOR_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_lengthMult", DataType::Float},
        {"u_radius", DataType::Float},
    }, 

    { }, // attributes
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 vector[];
        uniform mat4 u_projMatrix;
        uniform float u_lengthMult;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        
        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main()   {

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
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();
        }
    )
};



const ShaderStageSpecification VECTOR_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
        {"u_baseColor", DataType::Vector3Float},
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
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        uniform vec3 u_baseColor;
        in vec3 tailView;
        in vec3 tipView;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        bool rayConeIntersection(vec3 rayStart, vec3 rayDir, vec3 coneBase, vec3 coneTip, float coneRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           float tipLengthFrac = 0.2;
           float tipWidthFrac = 0.6;
           float adjRadius = min(u_radius, length(tipView - tailView)*tipLengthFrac); // clip vector aspect ratio by shrinking width of small vectors (length is always an accurate representation of data)

           // Raycast to the cylinder 
           float tHit = LARGE_FLOAT();
           vec3 pHit = vec3(777,777,777);
           vec3 nHit =  vec3(777,777,777);
           vec3 cylEnd = tailView + (1. - tipLengthFrac) * (tipView - tailView);
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, cylEnd, tipWidthFrac * adjRadius, tHit, pHit, nHit);
          
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
           

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, u_baseColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};

// clang-format on

} // namespace render
} // namespace polyscope

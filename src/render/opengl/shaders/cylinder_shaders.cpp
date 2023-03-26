// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/cylinder_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification FLEX_CYLINDER_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", RenderDataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", RenderDataType::Vector3Float},
        {"a_position_tip", RenderDataType::Vector3Float},
    },

    {}, // textures

    // source
R"(
        ${ GLSL_VERSION }$

        in vec3 a_position_tail;
        in vec3 a_position_tip;
        uniform mat4 u_modelView;
        out vec4 position_tip;
        
        ${ VERT_DECLARATIONS }$
        
        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail, 1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);

            ${ VERT_ASSIGNMENTS }$
        }
)"
};

const ShaderStageSpecification FLEX_CYLINDER_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", RenderDataType::Matrix44Float},
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
        in vec4 position_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;

        ${ GEOM_DECLARATIONS }$

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {
            float tipRadius = u_radius;
            float tailRadius = u_radius;
            ${ CYLINDER_SET_RADIUS_GEOM }$

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dxTip = u_projMatrix * vec4(basisX * tipRadius, 0.);
            vec4 dyTip = u_projMatrix * vec4(basisY * tipRadius, 0.);
            vec4 dxTail = u_projMatrix * vec4(basisX * tailRadius, 0.);
            vec4 dyTail = u_projMatrix * vec4(basisY * tailRadius, 0.);

            vec4 p1 = tailProj - dxTail - dyTail;
            vec4 p2 = tailProj + dxTail - dyTail;
            vec4 p3 = tailProj - dxTail + dyTail;
            vec4 p4 = tailProj + dxTail + dyTail;
            vec4 p5 = tipProj - dxTip - dyTip;
            vec4 p6 = tipProj + dxTip - dyTip;
            vec4 p7 = tipProj - dxTip + dyTip;
            vec4 p8 = tipProj + dxTip + dyTip;
            
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


const ShaderStageSpecification FLEX_CYLINDER_FRAG_SHADER = {
    
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
        bool rayTaperedCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRadTail, float cylRadTip, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        
        ${ FRAG_DECLARATIONS }$

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           
           float tipRadius = u_radius;
           float tailRadius = u_radius;
           ${ CYLINDER_SET_RADIUS_FRAG }$

           // Raycast to the cylinder
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayTaperedCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, tailRadius, tipRadius, tHit, pHit, nHit);
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

const ShaderReplacementRule CYLINDER_PROPAGATE_VALUE (
    /* rule name */ "CYLINDER_PROPAGATE_VALUE",
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

// like propagate value, but takes two values at tip and tail and linearly interpolates
const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_VALUE (
    /* rule name */ "CYLINDER_PROPAGATE_BLEND_VALUE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_value_tail;
          in float a_value_tip;
          out float a_valueTailToGeom;
          out float a_valueTipToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_valueTailToGeom = a_value_tail;
          a_valueTipToGeom = a_value_tip;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_valueTailToGeom[];
          in float a_valueTipToGeom[];
          out float a_valueTailToFrag;
          out float a_valueTipToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_valueTailToFrag = a_valueTailToGeom[0]; 
          a_valueTipToFrag = a_valueTipToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_valueTailToFrag;
          in float a_valueTipToFrag;
          float length2(vec3 x);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
          float shadeValue = mix(a_valueTailToFrag, a_valueTipToFrag, tEdge);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_value_tail", RenderDataType::Float},
      {"a_value_tip", RenderDataType::Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule CYLINDER_PROPAGATE_COLOR (
    /* rule name */ "CYLINDER_PROPAGATE_COLOR",
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

// like propagate color, but takes two values at tip and taail and linearly interpolates
const ShaderReplacementRule CYLINDER_PROPAGATE_BLEND_COLOR (
    /* rule name */ "CYLINDER_PROPAGATE_BLEND_COLOR",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color_tail;
          in vec3 a_color_tip;
          out vec3 a_colorTailToGeom;
          out vec3 a_colorTipToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorTailToGeom = a_color_tail;
          a_colorTipToGeom = a_color_tip;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorTailToGeom[];
          in vec3 a_colorTipToGeom[];
          out vec3 a_colorTailToFrag;
          out vec3 a_colorTipToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorTailToFrag = a_colorTailToGeom[0]; 
          a_colorTipToFrag = a_colorTipToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in vec3 a_colorTailToFrag;
          in vec3 a_colorTipToFrag;
          float length2(vec3 x);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
          vec3 shadeColor = mix(a_colorTailToFrag, a_colorTipToFrag, tEdge);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color_tail", RenderDataType::Vector3Float},
      {"a_color_tip", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule CYLINDER_CULLPOS_FROM_MID (
    /* rule name */ "CYLINDER_CULLPOS_FROM_MID",
    { /* replacement sources */
      {"GLOBAL_FRAGMENT_FILTER_PREP", R"(
          vec3 cullPos = 0.5 * (tailView + tipView);
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

// data for picking
const ShaderReplacementRule CYLINDER_PROPAGATE_PICK (
    /* rule name */ "CYLINDER_PROPAGATE_PICK",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in vec3 a_color_tail;
          in vec3 a_color_tip;
          in vec3 a_color_edge;
          out vec3 a_colorTailToGeom;
          out vec3 a_colorTipToGeom;
          out vec3 a_colorEdgeToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_colorTailToGeom = a_color_tail;
          a_colorTipToGeom = a_color_tip;
          a_colorEdgeToGeom = a_color_edge;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in vec3 a_colorTailToGeom[];
          in vec3 a_colorTipToGeom[];
          in vec3 a_colorEdgeToGeom[];
          flat out vec3 a_colorTailToFrag;
          flat out vec3 a_colorTipToFrag;
          flat out vec3 a_colorEdgeToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_colorTailToFrag = a_colorTailToGeom[0]; 
          a_colorTipToFrag = a_colorTipToGeom[0]; 
          a_colorEdgeToFrag = a_colorEdgeToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          flat in vec3 a_colorTailToFrag;
          flat in vec3 a_colorTipToFrag;
          flat in vec3 a_colorEdgeToFrag;
          float length2(vec3 x);
        )"},
      {"GENERATE_SHADE_VALUE", R"(
          float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
          float endWidth = 0.2;
          vec3 shadeColor;
          if(tEdge < endWidth) {
            shadeColor = a_colorTailToFrag;
          } else if (tEdge < (1.0f - endWidth)) {
            shadeColor = a_colorEdgeToFrag;
          } else {
            shadeColor = a_colorTipToFrag;
          }
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_color_tail", RenderDataType::Vector3Float},
      {"a_color_tip", RenderDataType::Vector3Float},
      {"a_color_edge", RenderDataType::Vector3Float},
    },
    /* textures */ {}
);

const ShaderReplacementRule CYLINDER_VARIABLE_SIZE (
    /* rule name */ "CYLINDER_VARIABLE_SIZE",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          in float a_tipRadius;
          in float a_tailRadius;
          out float a_tipRadiusToGeom;
          out float a_tailRadiusToGeom;
        )"},
      {"VERT_ASSIGNMENTS", R"(
          a_tipRadiusToGeom = a_tipRadius;
          a_tailRadiusToGeom = a_tailRadius;
        )"},
      {"GEOM_DECLARATIONS", R"(
          in float a_tipRadiusToGeom[];
          in float a_tailRadiusToGeom[];
          out float a_tipRadiusToFrag;
          out float a_tailRadiusToFrag;
        )"},
      {"GEOM_PER_EMIT", R"(
          a_tipRadiusToFrag = a_tipRadiusToGeom[0]; 
          a_tailRadiusToFrag = a_tailRadiusToGeom[0]; 
        )"},
      {"FRAG_DECLARATIONS", R"(
          in float a_tipRadiusToFrag;
          in float a_tailRadiusToFrag;
        )"},
      {"CYLINDER_SET_RADIUS_GEOM", R"(
          tipRadius *= a_tipRadiusToGeom[0];
          tailRadius *= a_tailRadiusToGeom[0];
        )"},
      {"CYLINDER_SET_RADIUS_FRAG", R"(
          tipRadius *= a_tipRadiusToFrag;
          tailRadius *= a_tailRadiusToFrag;
        )"},
    },
    /* uniforms */ {},
    /* attributes */ {
      {"a_tipRadius", RenderDataType::Float},
      {"a_tailRadius", RenderDataType::Float},
    },
    /* textures */ {}
);

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

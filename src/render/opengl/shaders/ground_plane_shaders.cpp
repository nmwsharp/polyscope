// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification GROUND_PLANE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_viewMatrix", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
       {"u_groundHeight", DataType::Float},
       {"u_basisZ", DataType::Vector3Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector4Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      uniform float u_groundHeight;
      uniform vec3 u_basisZ;
      in vec4 a_position;
      out vec4 PositionWorldHomog;
      out vec3 viewNormal;
      out vec3 viewPos;

      void main()
      {
          vec4 adjustedPosition = a_position + vec4(u_basisZ, 0.) * u_groundHeight * a_position.w;
          gl_Position = u_projMatrix * u_viewMatrix * adjustedPosition;
         
          PositionWorldHomog = adjustedPosition;
          
          viewNormal = mat3(u_viewMatrix) * u_basisZ;
          vec4 viewPos4 = u_viewMatrix * adjustedPosition;
          viewPos = vec3(viewPos4);
      }
    )
};

const ShaderStageSpecification GROUND_PLANE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
      {"u_lengthScale", DataType::Float},
      {"u_center", DataType::Vector3Float},
      {"u_basisX", DataType::Vector3Float},
      {"u_basisY", DataType::Vector3Float},
      {"u_viewportDim", DataType::Vector2Float},
      {"u_cameraHeight", DataType::Float},
      {"u_groundHeight", DataType::Float},
      {"u_roughness", DataType::Float},
      {"u_metallic", DataType::Float},
      {"u_F0", DataType::Float},
      {"u_ambientStrength", DataType::Float},
      {"u_lightStrength", DataType::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_ground", 2},
        {"t_mirrorImage", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,

      uniform sampler2D t_ground;
      uniform sampler2D t_mirrorImage;
      uniform mat4 u_viewMatrix;
      uniform float u_lengthScale;
      uniform vec3 u_center;
      uniform vec3 u_basisX;
      uniform vec3 u_basisY;
      uniform vec2 u_viewportDim;
      uniform float u_cameraHeight;
      uniform float u_groundHeight;
      uniform float u_roughness;
      uniform float u_metallic;
      uniform float u_F0;
      uniform float u_ambientStrength;
      uniform float u_lightStrength;
      in vec4 PositionWorldHomog;
      in vec3 viewNormal;
      in vec3 viewPos; 
      out vec4 colorVal; 
      
      vec3 computeLightingPBR(vec3 position, vec3 N, vec3 albedo, float roughness, float metallic, float F0val, float lightStrength, float ambientStrength);

      vec4 blurMirrorSample() {
        vec2 screenCoords = vec2(gl_FragCoord.x, gl_FragCoord.y);

        vec4 mirrorImage =
          texture(t_mirrorImage, screenCoords / u_viewportDim) * .4 + 
          texture(t_mirrorImage, (screenCoords + vec2(+1.0, +1.0)) / u_viewportDim) * .15 + 
          texture(t_mirrorImage, (screenCoords + vec2(+1.0, -1.0)) / u_viewportDim) * .15 + 
          texture(t_mirrorImage, (screenCoords + vec2(-1.0, +1.0)) / u_viewportDim) * .15 + 
          texture(t_mirrorImage, (screenCoords + vec2(-1.0, -1.0)) / u_viewportDim) * .15;

        return mirrorImage;
      }

      void main() {

        vec3 coord = PositionWorldHomog.xyz / PositionWorldHomog.w - u_center;
        coord /= u_lengthScale * .5;
        vec2 coord2D = vec2(dot(u_basisX, coord), dot(u_basisY, coord));

        // Checker stripes
        float modDist = min(min(mod(coord2D.x, 1.0), mod(coord2D.y, 1.0)), min(mod(-coord2D.x, 1.0), mod(-coord2D.y, 1.0)));
        float stripeBlendFac = smoothstep(0.005, .01, modDist);
        vec4 baseColor = mix(texture(t_ground, coord2D), vec4(.88, .88, .88, 1.), .4); 
        vec4 groundColor = mix( vec4(baseColor.xyz * .2, 1.0), baseColor, stripeBlendFac);
        vec3 shadedGroundColor = computeLightingPBR(viewPos, viewNormal, groundColor.rgb, u_roughness, u_metallic, u_F0, u_lightStrength, u_ambientStrength);

        // Mirror image
        //vec2 screenCoords = vec2(gl_FragCoord.x / u_viewportDim.x, gl_FragCoord.y / u_viewportDim.y);
        //vec4 mirrorImage = texture(t_mirrorImage, screenCoords);
        vec4 mirrorImage = blurMirrorSample();

        // Combined color
        vec3 combinedColor = mix(shadedGroundColor.rgb, mirrorImage.rgb * mirrorImage.w, .2 * mirrorImage.w);
        
        // Fade off far away
        float distFromCenter = length(coord2D);
        float distFadeFactor = 1.0 - smoothstep(8.0, 8.5, distFromCenter);
        float viewFromBelowFadeFactor = smoothstep(0, .1, (u_cameraHeight - u_groundHeight) / u_lengthScale);
        if(viewFromBelowFadeFactor == 0.) {
          discard;
        }
        float fadeFactor = min(distFadeFactor, viewFromBelowFadeFactor);
         
        colorVal = vec4(combinedColor, fadeFactor);
      }

    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

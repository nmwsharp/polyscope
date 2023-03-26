// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/ground_plane_shaders.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

// clang-format off

const ShaderStageSpecification GROUND_PLANE_VERT_SHADER =  {
    
    ShaderStageType::Vertex,
    
    // uniforms
    {
       {"u_viewMatrix", RenderDataType::Matrix44Float},
       {"u_projMatrix", RenderDataType::Matrix44Float},
       {"u_groundHeight", RenderDataType::Float},
       {"u_basisZ", RenderDataType::Vector3Float},
    },

    // attributes
    {
        {"a_position", RenderDataType::Vector4Float},
    },
    
    {}, // textures

    // source
R"(
      ${ GLSL_VERSION }$

      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      uniform float u_groundHeight;
      uniform vec3 u_basisZ;
      in vec4 a_position;
      out vec4 PositionWorldHomog;

      void main()
      {
          vec4 adjustedPosition = a_position + vec4(u_basisZ, 0.) * u_groundHeight * a_position.w;
          gl_Position = u_projMatrix * u_viewMatrix * adjustedPosition;
          PositionWorldHomog = adjustedPosition;
          vec4 viewPos4 = u_viewMatrix * adjustedPosition;
      }
)"
};

const ShaderStageSpecification GROUND_PLANE_TILE_FRAG_SHADER= {
    
    ShaderStageType::Fragment,

    { // uniforms
      {"u_lengthScale", RenderDataType::Float},
      {"u_center", RenderDataType::Vector3Float},
      {"u_basisX", RenderDataType::Vector3Float},
      {"u_basisY", RenderDataType::Vector3Float},
      {"u_viewportDim", RenderDataType::Vector2Float},
      {"u_cameraHeight", RenderDataType::Float},
      {"u_groundHeight", RenderDataType::Float},
      {"u_upSign", RenderDataType::Float}
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_ground", 2},
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      uniform sampler2D t_ground;
      uniform mat4 u_viewMatrix;
      uniform float u_lengthScale;
      uniform vec3 u_center;
      uniform vec3 u_basisX;
      uniform vec3 u_basisY;
      uniform vec2 u_viewportDim;
      uniform float u_cameraHeight;
      uniform float u_groundHeight;
      uniform float u_upSign;
      in vec4 PositionWorldHomog;
      layout(location = 0) out vec4 outputF;
        
      ${ FRAG_DECLARATIONS }$

      float orenNayarDiffuse( vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, float albedo);
      float specular( vec3 N, vec3 L, vec3 E, float shininess );

      void main()
      {
        float depth = gl_FragCoord.z;
        ${ GLOBAL_FRAGMENT_FILTER }$

        vec3 coord = PositionWorldHomog.xyz / PositionWorldHomog.w - u_center;
        coord /= u_lengthScale * .5;
        vec2 coord2D = vec2(dot(u_basisX, coord), dot(u_basisY, coord));

        // Checker stripes
        float modDist = min(min(mod(coord2D.x, 1.0), mod(coord2D.y, 1.0)), min(mod(-coord2D.x, 1.0), mod(-coord2D.y, 1.0)));
        float stripeBlendFac = smoothstep(0.005, .01, modDist);
        vec4 baseColor = mix(texture(t_ground, 0.5 * coord2D), vec4(.88, .88, .88, 1.), .5); 
        vec4 groundColor = mix( vec4(baseColor.xyz * .2, 1.0), baseColor, stripeBlendFac);

        // Ground color
        vec3 color3 = groundColor.rgb + 0. * u_viewportDim.x; // silly usage to avoid optimizing out

        // Lighting stuff
        vec4 posCameraSpace4 = u_viewMatrix * PositionWorldHomog;
        vec3 posCameraSpace = posCameraSpace4.xyz / posCameraSpace4.w;
        vec3 normalCameraSpace = mat3(u_viewMatrix) * vec3(0., 1., 0.);
        vec3 eyeCameraSpace = vec3(0., 0., 0.);
        vec3 lightPosCameraSpace = vec3(5., 5., -5.) * u_lengthScale;
        vec3 lightDir = normalize(lightPosCameraSpace - posCameraSpace);
        vec3 eyeDir = normalize(eyeCameraSpace - posCameraSpace);
        
        // Fade off far away
        float distFromCenter = length(coord2D);
        float distFadeFactor = 1.0 - smoothstep(8.0, 8.5, distFromCenter);
        float viewFromBelowFadeFactor = smoothstep(0, .1, u_upSign * (u_cameraHeight - u_groundHeight) / u_lengthScale);
        float fadeFactor = min(distFadeFactor, viewFromBelowFadeFactor);
        if(fadeFactor <= 0.) discard;
        vec4 color = vec4(color3, fadeFactor);
      
        // NOTE: parameters swapped from comments.. which is correct?
        float coloredBrightness = 1.2 * orenNayarDiffuse(eyeDir, lightDir, normalCameraSpace, .05, 1.0) + .3;
        float whiteBrightness = .25 * specular(normalCameraSpace, lightDir, eyeDir, 12.);

        vec4 lightColor = vec4(color.xyz * coloredBrightness + vec3(1., 1., 1.) * whiteBrightness, color.w);
        outputF = lightColor;
      }

)"
};

const ShaderStageSpecification GROUND_PLANE_TILE_REFLECT_FRAG_SHADER = {
    
    ShaderStageType::Fragment,

    { // uniforms
      {"u_lengthScale", RenderDataType::Float},
      {"u_center", RenderDataType::Vector3Float},
      {"u_basisX", RenderDataType::Vector3Float},
      {"u_basisY", RenderDataType::Vector3Float},
      {"u_viewportDim", RenderDataType::Vector2Float},
      {"u_cameraHeight", RenderDataType::Float},
      {"u_groundHeight", RenderDataType::Float},
      {"u_upSign", RenderDataType::Float}
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
R"(
      ${ GLSL_VERSION }$

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
      uniform float u_upSign;
      in vec4 PositionWorldHomog;
      layout(location = 0) out vec4 outputF;
      
      ${ FRAG_DECLARATIONS }$

      float orenNayarDiffuse( vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, float albedo);
      float specular( vec3 N, vec3 L, vec3 E, float shininess );

      vec4 sampleMirror() {
        vec2 screenCoords = vec2(gl_FragCoord.x, gl_FragCoord.y);
        vec4 mirrorImage = texture(t_mirrorImage, screenCoords / u_viewportDim) ;
        return mirrorImage;
      }

      void main()
      {
        float depth = gl_FragCoord.z;
        ${ GLOBAL_FRAGMENT_FILTER }$

        vec3 coord = PositionWorldHomog.xyz / PositionWorldHomog.w - u_center;
        coord /= u_lengthScale * .5;
        vec2 coord2D = vec2(dot(u_basisX, coord), dot(u_basisY, coord));

        // Checker stripes
        float modDist = min(min(mod(coord2D.x, 1.0), mod(coord2D.y, 1.0)), min(mod(-coord2D.x, 1.0), mod(-coord2D.y, 1.0)));
        float stripeBlendFac = smoothstep(0.005, .01, modDist);
        vec4 baseColor = mix(texture(t_ground, 0.5 * coord2D), vec4(.88, .88, .88, 1.), .5); 
        vec4 groundColor = mix(vec4(baseColor.xyz * .2, 1.0), baseColor, stripeBlendFac);

        // Mirror image
        //vec2 screenCoords = vec2(gl_FragCoord.x / u_viewportDim.x, gl_FragCoord.y / u_viewportDim.y);
        //vec4 mirrorImage = texture(t_mirrorImage, screenCoords);
        vec4 mirrorImage = sampleMirror();

        // Ground color
        vec3 color3 = mix(groundColor.rgb, mirrorImage.rgb * mirrorImage.w, .2 * mirrorImage.w);

        // Lighting stuff
        vec4 posCameraSpace4 = u_viewMatrix * PositionWorldHomog;
        vec3 posCameraSpace = posCameraSpace4.xyz / posCameraSpace4.w;
        vec3 normalCameraSpace = mat3(u_viewMatrix) * vec3(0., 1., 0.);
        vec3 eyeCameraSpace = vec3(0., 0., 0.);
        vec3 lightPosCameraSpace = vec3(5., 5., -5.) * u_lengthScale;
        vec3 lightDir = normalize(lightPosCameraSpace - posCameraSpace);
        vec3 eyeDir = normalize(eyeCameraSpace - posCameraSpace);
        
        // Fade off far away
        float distFromCenter = length(coord2D);
        float distFadeFactor = 1.0 - smoothstep(8.0, 8.5, distFromCenter);
        float viewFromBelowFadeFactor = smoothstep(0, .1, u_upSign * (u_cameraHeight - u_groundHeight) / u_lengthScale);
        float fadeFactor = min(distFadeFactor, viewFromBelowFadeFactor);
        if(fadeFactor <= 0.) discard;
        vec4 color = vec4(color3, fadeFactor);
      
        // NOTE: parameters swapped from comments.. which is correct?
        float coloredBrightness = 1.2 *orenNayarDiffuse(eyeDir, lightDir, normalCameraSpace, .05, 1.0) + .3;
        float whiteBrightness = .25 * specular(normalCameraSpace, lightDir, eyeDir, 12.);

        vec4 lightColor = vec4(color.xyz * coloredBrightness + vec3(1., 1., 1.) * whiteBrightness, color.w);
        outputF = lightColor;
      }

)"
};

const ShaderStageSpecification GROUND_PLANE_SHADOW_FRAG_SHADER = {
    
    ShaderStageType::Fragment,

    { // uniforms
      {"u_lengthScale", RenderDataType::Float},
      {"u_viewportDim", RenderDataType::Vector2Float},
      {"u_shadowDarkness", RenderDataType::Float},
      {"u_cameraHeight", RenderDataType::Float},
      {"u_groundHeight", RenderDataType::Float},
      {"u_upSign", RenderDataType::Float}
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_shadow", 2},
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      uniform sampler2D t_shadow;
      uniform mat4 u_viewMatrix;
      uniform vec2 u_viewportDim;
      uniform float u_lengthScale;
      uniform float u_shadowDarkness;
      uniform float u_cameraHeight;
      uniform float u_groundHeight;
      uniform float u_upSign;
      in vec4 PositionWorldHomog;
      layout(location = 0) out vec4 outputF;
      
      ${ FRAG_DECLARATIONS }$

      void main()
      {
        float depth = gl_FragCoord.z;
        ${ GLOBAL_FRAGMENT_FILTER }$

        vec2 screenCoords = vec2(gl_FragCoord.x / u_viewportDim.x, gl_FragCoord.y / u_viewportDim.y);
        float shadowVal = texture(t_shadow, screenCoords).r;
        shadowVal = pow(clamp(shadowVal, 0., 1.), 0.25);

        float shadowMax = u_shadowDarkness + 0. * PositionWorldHomog.x;  // use PositionWorldHomog to prevent silly optimizing out
        vec3 groundColor = vec3(0., 0., 0.);
        //vec3 groundColor = vec3(1., 0., 0.);

        // Fade off when viewed from below
        float viewFromBelowFadeFactor = smoothstep(0, .1, u_upSign * (u_cameraHeight - u_groundHeight) / u_lengthScale);
        float fadeFactor = viewFromBelowFadeFactor;
        if(fadeFactor <= 0.) discard;

        outputF = vec4(groundColor, shadowMax*shadowVal*fadeFactor);
        //outputF = vec4(groundColor, shadowMax*shadowVal);
        //groundColor.r = shadowVal; 
        //outputF = vec4(groundColor, 1.);
      }

)"
};

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

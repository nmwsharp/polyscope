// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

// clang-format off

namespace polyscope {
namespace render{

const ShaderStageSpecification PBR_DEFERRED_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_exposure", DataType::Float},
        {"u_ambientStrength", DataType::Float},
        {"u_lightStrength", DataType::Float},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_albedo", 2},
      {"t_material", 2},
      {"t_viewPos", 2},
      {"t_viewNormal", 2},
    },
    
    // source 
    // PBR Lighting functions below are copyright Joey de Vries at learnopengl.com, used under CC-BY-4.0
    POLYSCOPE_GLSL_DEFERRED(330 core,

      in vec2 tCoord;
      uniform sampler2D t_albedo;
      uniform sampler2D t_material;
      uniform sampler2D t_viewPos;
      uniform sampler2D t_viewNormal;
      uniform float u_exposure;
      uniform float u_ambientStrength;
      uniform float u_lightStrength;

      // Forward declarations for pbr functions
      vec3 computeLightingPBR(vec3 position, vec3 N, vec3 albedo, float roughness, float metallic, float F0val, float lightStrength, float ambientStrength);

      void main() {

        // == Gather values
       
        // albedos
        vec4 albedo4 = texture(t_albedo, tCoord);
        vec3 albedo = albedo4.rgb;
        float alpha = albedo4.a;

        // material parameters
        vec4 material4 = texture(t_material, tCoord);
        float roughness = material4.x;
        float metallic = material4.y;
        float F0val = material4.z;
       
        // positions
        vec3 position = texture(t_viewPos, tCoord).rgb;

        // normals
        vec3 N = texture(t_viewNormal, tCoord).rgb;

        vec3 resultColor = computeLightingPBR(position, N, albedo, roughness, metallic, F0val, u_lightStrength, u_ambientStrength);
       
        // tonemapping
        resultColor = vec3(1.0) - exp(-resultColor * u_exposure);

        // gamma correction
        resultColor = pow(resultColor, vec3(1.0/2.2));  
       
        gFinal = vec4(resultColor, alpha);
    }  
    )
};


// clang-format on

} // namespace render
} // namespace polyscope

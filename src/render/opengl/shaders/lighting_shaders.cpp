// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

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
    
    // output location
    "",
    
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
      float DistributionGGX(vec3 N, vec3 H, float roughness);
      float GeometrySchlickGGX(float NdotV, float roughness);
      float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
      vec3 FresnelSchlick(float cosTheta, vec3 F0);
      float softpos(float val);
      float softpos(float val, float low, float high);

      const float PI = 3.14159265359;

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
        N = normalize(N);

        // compute some intermediate values
        vec3 F0 = vec3(F0val);  // Fresnel constant (plastic-ish)
        F0 = mix(F0, albedo, metallic);
        vec3 V = normalize(-position);
        
        // if the normals are facing away from the camera, flip and take note 
        float flipsign = sign(dot(N,V));
        N = N * flipsign; // TODO problem if == 0?
        float backfaceFac = max(0., flipsign);

        vec3 Lo = vec3(0.0); // accumulate light results

        vec4 lightLocsX = vec4(1, 1, -1, -1);
        vec4 lightLocsY = vec4(1, -1, 1, -1);
        vec4 lightLocsZ = vec4(3.);
        for(int i = 0; i < 4; ++i) 
        {
            
            // calculate per-light radiance
            vec3 L = normalize(vec3(lightLocsX[i], lightLocsY[i], lightLocsZ[i]));
            vec3 H = normalize(V + L);
            vec3 radiance = u_lightStrength * vec3(1.,.9,.9); // lights at infinity, no attenuation
            
            // cook-torrance brdf
            float NDF = DistributionGGX(N, H, roughness);        
            float G = GeometrySmith(N, V, L, roughness);      
            //vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);       
            vec3 F = FresnelSchlick(softpos(dot(H, V)), F0);       
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;	  
            
            vec3 num = NDF * G * F;
            //float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
            float denom = 4.0 * softpos(dot(N, V)) * softpos(dot(N, L));
            vec3 specular = num / max(denom, 0.001);  
                
            // add to outgoing radiance Lo
            //float NdotL = max(dot(N, L), 0.0);
            float NdotL = softpos(dot(N, L), .1, .13);
            vec3 contrib = (kD * albedo / PI + specular * backfaceFac) * radiance * NdotL;
            Lo += contrib;
        }   
     
        // Add an ambient term
        float ao = 1.0;
        vec3 ambient = vec3(u_ambientStrength) * albedo * ao;
        vec3 resultColor = ambient + Lo;
     
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

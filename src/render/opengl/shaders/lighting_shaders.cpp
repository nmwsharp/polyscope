// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_engine.h"
#include "polyscope/render/shaders.h"

// clang-format off

namespace polyscope {
namespace render{

const ShaderStageSpecification MAP_LIGHT_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_exposure", DataType::Float},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_image", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform float u_exposure;
      layout (location = 0) out vec4 outputVal;

      // Forward declarations for pbr functions

      void main() {

        vec4 color4 = texture(t_image, tCoord);
        vec3 color = color4.rgb;
        float alpha = color4.a;

        // tonemapping
        color = vec3(1.0) - exp(-color * u_exposure);
        
        // gamma correction
        color = pow(color, vec3(1.0/2.2));  
       
        outputVal = vec4(color, alpha);
    }  
    )
};

const ShaderStageSpecification SPLIT_SPECULAR_PRECOMPUTE_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { },

    
    // source 
    POLYSCOPE_GLSL(330 core,
      
      in vec2 tCoord;
      layout (location = 0) out vec4 outputVal;
      
      const float PI = 3.14159265359;

      // PBR Lighting functions below are copyright Joey de Vries at learnopengl.com, used under CC-BY-4.0

      float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

      vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
          float a = roughness*roughness;
        
          float phi = 2.0 * PI * Xi.x;
          float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
          float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
        
          // from spherical coordinates to cartesian coordinates
          vec3 H;
          H.x = cos(phi) * sinTheta;
          H.y = sin(phi) * sinTheta;
          H.z = cosTheta;
        
          // from tangent-space vector to world-space sample vector
          vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
          vec3 tangent   = normalize(cross(up, N));
          vec3 bitangent = cross(N, tangent);
        
          vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
          return normalize(sampleVec);
      }  

      float VanDerCorpus(uint n, uint base) {
          float invBase = 1.0 / float(base);
          float denom   = 1.0;
          float result  = 0.0;

          for(uint i = 0u; i < 32u; ++i) {
              if(n > 0u) {
                  denom   = mod(float(n), 2.0);
                  result += denom * invBase;
                  invBase = invBase / 2.0;
                  n       = uint(float(n) / 2.0);
              }
          }

          return result;
      }

      vec2 HammersleyNoBitOps(uint i, uint N) {
          return vec2(float(i)/float(N), VanDerCorpus(i, 2u));
      }

      vec2 splitSpecularPrecompute(float NdotV, float roughness) {
          vec3 V;
          V.x = sqrt(1.0 - NdotV*NdotV);
          V.y = 0.0;
          V.z = NdotV;

          float A = 0.0;
          float B = 0.0;

          vec3 N = vec3(0.0, 0.0, 1.0);

          const uint SAMPLE_COUNT = 1024u;
          for(uint i = 0u; i < SAMPLE_COUNT; ++i)
          {
              vec2 Xi = HammersleyNoBitOps(i, SAMPLE_COUNT);
              vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
              vec3 L  = normalize(2.0 * dot(V, H) * H - V);

              float NdotL = max(L.z, 0.0);
              float NdotH = max(H.z, 0.0);
              float VdotH = max(dot(V, H), 0.0);

              if(NdotL > 0.0)
              {
                  float G = GeometrySmith(N, V, L, roughness);
                  float G_Vis = (G * VdotH) / (NdotH * NdotV);
                  float Fc = pow(1.0 - VdotH, 5.0);

                  A += (1.0 - Fc) * G_Vis;
                  B += Fc * G_Vis;
              }
          }
          A /= float(SAMPLE_COUNT);
          B /= float(SAMPLE_COUNT);
					return vec2(A, B);
      }

      void main() {
				vec2 integratedBRDF = splitSpecularPrecompute(tCoord.x, tCoord.y);
				outputVal = vec4(integratedBRDF.x, integratedBRDF.y, 0., 1.);
      }  
    )
};

/*
const ShaderStageSpecification PRECOMPUTE_PBR_ENV_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_exposure", DataType::Float},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_image", 2},
    },
    
    // source 
    POLYSCOPE_GLSL(330 core,

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform float u_exposure;
      layout (location = 0) out vec4 outputVal;
          
      const float PI = 3.14159265359;

      vec3 pbrSampleResult(vec3 inDir, vec3 outDir) {
          
          // calculate per-light radiance
          vec3 L = outDir;
          vec3 H = normalize(V + L);

          // sample the envmap to get the light
          vec3 radiance = lightStrength;

          // compute some intermediate values
          vec3 F0 = vec3(u_F0val);  // Fresnel constant 
          F0 = mix(F0, albedo, metallic);
          vec3 V = inDir;
          
          // cook-torrance brdf
          float NDF = DistributionGGX(N, H, u_roughness);        
          float G = GeometrySmith(N, V, L, u_roughness);      
          vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);       
          
          vec3 kS = F;
          vec3 kD = vec3(1.0) - kS;
          kD *= 1.0 - u_metallic;	  
          
          vec3 num = NDF * G * F;
          float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
          vec3 specular = num / max(denom, 0.001);  
              
          // add to outgoing radiance Lo
          float NdotL = max(dot(N, L), 0.0);
          vec3 contrib = (kD * albedo / PI + specular * backfaceFac) * radiance * NdotL;
          return contrib;
      }

      void main() {

        vec3 lightSum = vec3(0.0); // accumulate light results

        // Build a tangent frame
        

        const uint SAMPLE_THETA = 100u;
        const uint SAMPLE_PHI = 10u;
        float totalWeight = 0.0;   
        vec3 prefilteredColor = vec3(0.0);     
        float delTheta = 2. * PI / SAMPLE_THETA;
        float delPhi = PI / 2. / SAMPLE_PHI;
        for(uint i_th = 0u; i_th < SAMPLE_THETA ; i_th++) {
          for(uint i_ph = 0u; i_ph < SAMPLE_PHI ; i_ph++) {

          }
        }

        
        outputVal = vec4(color, alpha);

        return resultColor;
    }  
    )
};
*/



// clang-format on

} // namespace render
} // namespace polyscope

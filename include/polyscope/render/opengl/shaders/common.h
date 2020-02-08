// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// This file defines common POLYSCOPE_GLSL constants and routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

#include "polyscope/render/opengl/gl_engine.h"

namespace polyscope {
namespace render {

// clang-format off

const char* shaderCommonSource = R"(
#version 410

const vec3 RGB_TEAL     = vec3(0., 178./255., 178./255.);
const vec3 RGB_BLUE     = vec3(150./255., 154./255., 255./255.);
const vec3 RGB_SKYBLUE  = vec3(152./255., 158./255., 200./255.);
const vec3 RGB_ORANGE   = vec3(1., 0.45, 0.);
const vec3 RGB_BLACK    = vec3(0., 0., 0.);
const vec3 RGB_WHITE    = vec3(1., 1., 1.);
const vec3 RGB_RED      = vec3(0.8, 0., 0.);
const vec3 RGB_DARKGRAY = vec3( .2, .2, .2 );
const vec3 RGB_DARKRED  = vec3( .2, .0, .0 );

float orenNayarDiffuse(
  vec3 lightDirection,
  vec3 viewDirection,
  vec3 surfaceNormal,
  float roughness,
  float albedo) {
  
  float LdotV = dot(lightDirection, viewDirection);
  float NdotL = dot(lightDirection, surfaceNormal);
  float NdotV = dot(surfaceNormal, viewDirection);

  float s = LdotV - NdotL * NdotV;
  float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

  float sigma2 = roughness * roughness;
  float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
  float B = 0.45 * sigma2 / (sigma2 + 0.09);

  return albedo * max(0.0, NdotL) * (A + B * s / t) / 3.14159;
}


float specular( vec3 N, vec3 L, vec3 E, float shininess ) {
   vec3 R = 2.*dot(L,N)*N - L;
   return pow( max( 0., dot( R, E )), shininess );
}

float fresnel( vec3 N, vec3 E ) {
   const float sharpness = 10.;
   float NE = max( 0., dot( N, E ));
   return pow( sqrt( 1. - NE*NE ), sharpness );
}

vec3 gammaCorrect( vec3 colorLinear )
{
   const float screenGamma = 2.2;
   return pow(colorLinear, vec3(1.0/screenGamma));
}

vec3 undoGammaCorrect( vec3 colorLinear )
{
   const float screenGamma = 2.2;
   return pow(colorLinear, vec3(screenGamma));
}
      

vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b) {
  normal = normalize(normal);
  normal.y = -normal.y;
  vec2 matUV = normal.xy/2.0 + vec2(.5, .5);
  
  vec3 mat_r = undoGammaCorrect(texture(t_mat_r, matUV).rgb);
  vec3 mat_g = undoGammaCorrect(texture(t_mat_g, matUV).rgb);
  vec3 mat_b = undoGammaCorrect(texture(t_mat_b, matUV).rgb);
  vec3 colorCombined = gammaCorrect(color.r * mat_r + color.g * mat_g + color.b * mat_b);

  return vec4(colorCombined, 1.0);
}


// PBR Lighting functions below are copyright Joey de Vries at learnopengl.com, used under CC-BY-4.0

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    const float PI = 3.14159265359;
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
  // TODO add roughness
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec2 sphericalTexCoords(vec3 v) {
  const vec2 invAtan = vec2(0.1591, 0.3183);
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invAtan;
  uv += 0.5;
  return uv;
}

vec3 computeLightingPBR(vec3 position, vec3 N, vec3 albedo, float roughness, float metallic, float F0val, float lightStrength, float ambientStrength) {
  const float PI = 3.14159265359;

  // compute some intermediate values
  vec3 F0 = vec3(F0val);  // Fresnel constant (plastic-ish)
  F0 = mix(F0, albedo, metallic);
  vec3 V = normalize(-position);
  
  // if the normals are facing away from the camera, flip and take note 
  N = normalize(N);
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
      vec3 radiance = lightStrength * vec3(1.,.9,.9); // lights at infinity, no attenuation
      
      // cook-torrance brdf
      float NDF = DistributionGGX(N, H, roughness);        
      float G = GeometrySmith(N, V, L, roughness);      
      vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);       
      
      vec3 kS = F;
      vec3 kD = vec3(1.0) - kS;
      kD *= 1.0 - metallic;	  
      
      vec3 num = NDF * G * F;
      float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
      vec3 specular = num / max(denom, 0.001);  
          
      // add to outgoing radiance Lo
      float NdotL = max(dot(N, L), 0.0);
      vec3 contrib = (kD * albedo / PI + specular * backfaceFac) * radiance * NdotL;
      Lo += contrib;
  }   

  // Add an ambient term
  float ao = 1.0;
  vec3 ambient = vec3(ambientStrength) * albedo * ao;
  vec3 resultColor = ambient + Lo;

  return resultColor;
}


vec3 computeLightingPBR(vec3 position, vec3 N, vec3 albedo, float roughness, float metallic, float F0val, float lightStrength, float ambientStrength, sampler2D t_envDiffuse, sampler2D t_envSpecular, sampler2D t_specularPrecomp) {
  const float PI = 3.14159265359;

  vec3 viewDirN = normalize(N);
  vec2 sampleCoords = sphericalTexCoords(viewDirN);
  
  // compute some intermediate values
  vec3 F0 = vec3(F0val);  // Fresnel constant (plastic-ish)
  F0 = mix(F0, albedo, metallic);
  vec3 V = normalize(-position);
  
  // if the normals are facing away from the camera, flip and take note 
  N = normalize(N);
  float flipsign = sign(dot(N,V));
  N = N * flipsign; // TODO problem if == 0?
  float backfaceFac = max(0., flipsign);

  vec3 F = FresnelSchlick(max(dot(N, V), 0.0), F0);
  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = texture(t_envDiffuse, sampleCoords).rgb;
	vec3 diffuse = irradiance * albedo;

  vec2 envBRDF  = texture(t_specularPrecomp, vec2(max(dot(N, V), 0.0), roughness)).rg;
	//vec3 specularIncoming = texture(t_envSpecular, sampleCoords).rgb; FIXME
	vec3 specularIncoming = texture(t_envDiffuse, sampleCoords).rgb;
	vec3 specular = specularIncoming * (F * envBRDF.x + envBRDF.y);

  float ao = 1.0;
	vec3 result = (kD * diffuse + specular) * ao; 

  return result;
}



)";

// clang-format on

} // namespace render
} // namespace polyscope

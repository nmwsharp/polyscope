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

float softpos(float val, float threshLow, float threshHigh) {
  float b = smoothstep(threshHigh, threshLow, val) * threshLow;
  return max(val, 0.0) + b;
}

float softpos(float val) {
  return softpos(val, 0.05, 0.08);
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
    //float NdotV = max(dot(N, V), 0.0);
    //float NdotL = max(dot(N, L), 0.0);
    float NdotV = softpos(dot(N, V));
    float NdotL = softpos(dot(N, L));
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  


)";

// clang-format on

} // namespace render
} // namespace polyscope

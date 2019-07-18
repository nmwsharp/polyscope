// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// This file defines common POLYSCOPE_GLSL constants and routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

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
  normal.y = -normal.y;
  vec2 matUV = normal.xy/2.0 + vec2(.5, .5);
  
  vec3 mat_r = undoGammaCorrect(texture(t_mat_r, matUV).rgb);
  vec3 mat_g = undoGammaCorrect(texture(t_mat_g, matUV).rgb);
  vec3 mat_b = undoGammaCorrect(texture(t_mat_b, matUV).rgb);
  vec3 colorCombined = gammaCorrect(color.r * mat_r + color.g * mat_g + color.b * mat_b);

  return vec4(colorCombined, 1.0);
}



)";

// clang-format on

} // namespace gl
} // namespace polyscope

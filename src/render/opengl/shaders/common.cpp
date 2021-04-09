// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/shaders/common.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {


const char* shaderCommonSource = R"(

const vec3 RGB_TEAL     = vec3(0., 178./255., 178./255.);
const vec3 RGB_BLUE     = vec3(150./255., 154./255., 255./255.);
const vec3 RGB_SKYBLUE  = vec3(152./255., 158./255., 200./255.);
const vec3 RGB_ORANGE   = vec3(1., 0.45, 0.);
const vec3 RGB_BLACK    = vec3(0., 0., 0.);
const vec3 RGB_WHITE    = vec3(1., 1., 1.);
const vec3 RGB_RED      = vec3(0.8, 0., 0.);
const vec3 RGB_DARKGRAY = vec3( .2, .2, .2 );
const vec3 RGB_DARKRED  = vec3( .2, .0, .0 );

float LARGE_FLOAT() { return 1e25; }
float length2(vec3 a) { return dot(a,a); }

void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY) {
    basisX = vec3(1., 0., 0.);
    basisX -= dot(basisX, unitNormal) * unitNormal;
    if(abs(basisX.x) < 0.1) {
      basisX = vec3(0., 1., 0.);
      basisX -= dot(basisX, unitNormal) * unitNormal;
    }
    basisX = normalize(basisX);
    basisY = normalize(cross(unitNormal, basisX));
}

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

float luminance(vec3 v) {
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
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
      

vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k) {

  // ensure color is in range [0,1]
  color = clamp(color, vec3(0.), vec3(1.));

  normal = normalize(normal);
  normal.y = -normal.y;
  normal *= 0.98; // pull slightly inward, to reduce sampling artifacts near edges
  vec2 matUV = normal.xy/2.0 + vec2(.5, .5);
  
  vec3 mat_r = texture(t_mat_r, matUV).rgb;
  vec3 mat_g = texture(t_mat_g, matUV).rgb;
  vec3 mat_b = texture(t_mat_b, matUV).rgb;
  vec3 mat_k = texture(t_mat_k, matUV).rgb;
  vec3 colorCombined = color.r * mat_r + color.g * mat_g + color.b * mat_b + 
                       (1. - color.r - color.g - color.b) * mat_k;

  return colorCombined;
}

vec2 sphericalTexCoords(vec3 v) {
  const vec2 invMap = vec2(0.1591, 0.3183);
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invMap;
  uv += 0.5;
  return uv;
}

// Two useful references:
//   - https://stackoverflow.com/questions/38938498/how-do-i-convert-gl-fragcoord-to-a-world-space-point-in-a-fragment-shader
//   - https://stackoverflow.com/questions/11277501/how-to-recover-view-space-position-given-view-space-depth-value-and-ndc-xy

vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord) {
  vec4 ndcPos;
  ndcPos.xy = ((2.0 * fragCoord.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
  ndcPos.z = (2.0 * fragCoord.z - depthRange.x - depthRange.y) / (depthRange.y - depthRange.x);
  ndcPos.w = 1.0;

  vec4 clipPos = ndcPos / fragCoord.w;
  vec4 eyePos = invProjMat * clipPos;
  return eyePos.xyz / eyePos.w;
}

float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint) {
  vec4 clipPos = projMat * vec4(viewPoint, 1.); // only actually need one element of this result, could save work
  float z_ndc = clipPos.z / clipPos.w;
  float depth = (((depthRange.y-depthRange.x) * z_ndc) + depthRange.x + depthRange.y) / 2.0;
  return depth;
}

bool raySphereIntersection(vec3 rayStart, vec3 rayDir, vec3 sphereCenter, float sphereRad, out float tHit, out vec3 pHit, out vec3 nHit) {
    rayDir = normalize(rayDir);
    vec3 o = rayStart - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(o, rayDir);
    float c = dot(o,o) - sphereRad*sphereRad;
    float disc = b*b - 4*a*c;
    if(disc < 0){
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
    } else {
      tHit = (-b - sqrt(disc)) / (2.0*a);
      pHit = rayStart + tHit * rayDir;
      nHit = normalize(pHit - sphereCenter);
      return true;
    }
}

bool rayPlaneIntersection(vec3 rayStart, vec3 rayDir, vec3 planePos, vec3 planeDir, out float tHit, out vec3 pHit, out vec3 nHit) {
  
  float num = dot(planePos - rayStart, planeDir);
  float denom = dot(rayDir, planeDir);
  if(abs(denom) < 1e-6) {
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
  }

  tHit = num / denom;
  pHit = rayStart + tHit * rayDir;
  nHit = planeDir;
  return true;
}

bool rayDiskIntersection(vec3 rayStart, vec3 rayDir, vec3 planePos, vec3 planeDir, float diskRad, out float tHit, out vec3 pHit, out vec3 nHit) {
  
  float num = dot(planePos - rayStart, planeDir);
  float denom = dot(rayDir, planeDir);
  if(abs(denom) < 1e-6) {
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
  }

  tHit = num / denom;
  pHit = rayStart + tHit * rayDir;

  if(length2(pHit-planePos) > diskRad*diskRad) {
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
  }

  nHit = planeDir;

  return true;
}

bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit) {
    
    rayDir = normalize(rayDir);
    float cylLen = max(length(cylTip - cylTail), 1e-6);
    vec3 cylDir = (cylTip - cylTail) / cylLen;

    vec3 o = rayStart - cylTail;
    float d = dot(rayDir, cylDir);
    vec3 qVec = rayDir - d * cylDir;
    vec3 pVec = o - dot(o, cylDir)*cylDir;
    float a = length2(qVec);
    float b = 2.0 * dot(qVec, pVec);
    float c = length2(pVec) - cylRad*cylRad;
    float disc = b*b - 4*a*c;
    if(disc < 0){
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
    } 

    // Compute intersection with infinite cylinder
    tHit = (-b - sqrt(disc)) / (2.0*a);
    if(tHit < 0) tHit = (-b + sqrt(disc)) / (2.0*a); // try second intersection
    if(tHit < 0) {
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
    }
    pHit = rayStart + tHit * rayDir;
    nHit = pHit - cylTail;
    nHit = normalize(nHit - dot(cylDir, nHit)*cylDir); 

    // Check if intersection was outside finite cylinder
    if(dot(cylDir, pHit - cylTail) < 0 || dot(-cylDir, pHit - cylTip) < 0) {
      tHit = LARGE_FLOAT();
    }
    
    // Test start endcap
    float tHitTail;
    vec3 pHitTail;
    vec3 nHitTail;
    rayDiskIntersection(rayStart, rayDir, cylTail, -cylDir, cylRad, tHitTail, pHitTail, nHitTail);
    if(tHitTail < tHit) {
      tHit = tHitTail;
      pHit = pHitTail;
      nHit = nHitTail;
    }

    // Test end endcap
    float tHitTip;
    vec3 pHitTip;
    vec3 nHitTip;
    rayDiskIntersection(rayStart, rayDir, cylTip, cylDir, cylRad, tHitTip, pHitTip, nHitTip);
    if(tHitTip < tHit) {
      tHit = tHitTip;
      pHit = pHitTip;
      nHit = nHitTip;
    }

    return tHit >= 0;
}


bool rayConeIntersection(vec3 rayStart, vec3 rayDir, vec3 coneBase, vec3 coneTip, float coneRad, out float tHit, out vec3 pHit, out vec3 nHit) {

    rayDir = normalize(rayDir);
    float coneH = max(length(coneTip - coneBase), 1e-6);
    vec3 coneDir = (coneTip - coneBase) / coneH;
    
    vec3 O = rayStart;
    vec3 D = rayDir;
    vec3 C = coneTip;
    vec3 V = -coneDir;
    float cosT = coneH / sqrt(coneH*coneH + coneRad*coneRad);
    float DdotV = dot(D,V);
    vec3 CO = O-C;
    float COdotV = dot(CO, V);
    float a = DdotV*DdotV - cosT*cosT;
    float b = 2.0 * (DdotV*COdotV - dot(D, CO)*cosT*cosT);
    float c = COdotV*COdotV - dot(CO, CO)*cosT*cosT;
    float disc = b*b - 4*a*c;

    if(disc < 0){
      tHit = LARGE_FLOAT();
      pHit = vec3(777, 777, 777);
      nHit = vec3(777, 777, 777);
      return false;
    } 
    
    // Check first intersection
    // NOTE: The signs on the discriminant here and below are flipped from what you would expect,
    //       and I cannot figure out why. There must be some other matching sign flip elsewhere,
    //       or a bad bug in my understanding.
    tHit = (-b + sqrt(disc)) / (2.0*a); 
    pHit = rayStart + tHit * rayDir; 

    if((tHit < 0) || 
       (dot(pHit-coneTip,-coneDir) < 0.) || 
       (dot(pHit-coneBase, coneDir) < 0.)) {

      // try second intersection
      tHit = (-b - sqrt(disc)) / (2.0*a); 
      pHit = rayStart + tHit * rayDir;

    
      // Check second intersection
      if((tHit < 0) || 
         (dot(pHit-coneTip,-coneDir) < 0.) || 
         (dot(pHit-coneBase, coneDir) < 0.)) {
        tHit = LARGE_FLOAT();
        pHit = vec3(777, 777, 777);
        nHit = vec3(777, 777, 777);
        return false; 
      }
    }

    nHit = pHit - coneBase;
    vec3 sideDir = normalize(pHit - coneTip);
    nHit = normalize(nHit - dot(sideDir, nHit)*sideDir); 

    // Test base cap
    float tHitTail;
    vec3 pHitTail;
    vec3 nHitTail;
    rayDiskIntersection(rayStart, rayDir, coneBase, coneDir, coneRad, tHitTail, pHitTail, nHitTail);
    if(tHitTail < tHit) {
      tHit = tHitTail;
      pHit = pHitTail;
      nHit = nHitTail;
    }

    return true;
}

)";

}
} // namespace render
} // namespace polyscope


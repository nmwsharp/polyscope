// This file defines common GLSL constants and routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

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

// Common surface attributes
vec3 getSurfaceColor() { return RGB_TEAL; }
float getSurfaceShininess() { return 12.; }
vec3 getBackgroundColor() { return RGB_DARKGRAY; }

float diffuse( vec3 N, vec3 L ) {
   //return max( 0., dot( N, L ));
   return max( 0.3*dot( -N ,L ), dot( N, L ));  // nsharp: modification to give a little bit more character to surfaces viewed from behind
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

vec4 lightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye )
{
   float s = getSurfaceShininess();
   vec3 bgColor = getBackgroundColor();

   vec3 one = vec3( 1., 1., 1. );

   vec3 N = normalize( normal );
   vec3 L = normalize( light - position );
   vec3 E = normalize( eye - position );

   vec4 result;
   result.rgb = 0.05*color +
                0.9*diffuse(N,L)*color +
                0.2*specular(N,L,E,s)*one +
                .75*fresnel(N,E)*bgColor;
   result.a = 1.0;

   return result;
}

vec3 gammaCorrect( vec3 colorLinear )
{
   const float screenGamma = 2.2;
   return pow(colorLinear, vec3(1.0/screenGamma));
}

vec4 lightSurface( vec3 position, vec3 normal, vec3 light, vec3 eye )
{
   vec3 color = getSurfaceColor();
   vec4 result = lightSurface( position, normal, color, light, eye );
   //result.rgb = gammaCorrect( result.rgb );
   return result;
}

vec4 highlightSurface( vec3 position, vec3 normal, vec3 color, vec3 light, vec3 eye ) {

   vec4 result = lightSurface( position, normal, color, light, eye );
   result.rgb *= .5;
   return result;
}

vec4 highlightSurface( vec3 position, vec3 normal, vec3 light, vec3 eye ) {
   vec3 color = getSurfaceColor();
   return highlightSurface( position, normal, color, light, eye );
}

float getEdgeFactor(vec3 UVW) {
   // // uniform width lines
   // const float w = 1.5; // width
   // const float s = 8.; // hardness
   // float da = length(vec2(dFdx(UVW.x),dFdy(UVW.x)));
   // float db = length(vec2(dFdx(UVW.y),dFdy(UVW.y)));
   // float dc = length(vec2(dFdx(UVW.z),dFdy(UVW.z)));
   // float a = (UVW.x/(w*da)); a = pow(2., -2.*pow(a,s));
   // float b = (UVW.y/(w*db)); b = pow(2., -2.*pow(b,s));
   // float c = (UVW.z/(w*dc)); c = pow(2., -2.*pow(c,s));
   // return max(max(a,b),c);
   
   // // variable width lines
   // const float w = .03; // width
   // const float s = .085; // hardness
   // float da = length(vec2(dFdx(UVW.x),dFdy(UVW.x)));
   // float db = length(vec2(dFdx(UVW.y),dFdy(UVW.y)));
   // float dc = length(vec2(dFdx(UVW.z),dFdy(UVW.z)));
   // float a = UVW.x/w; a = pow( 2., -2.*pow(a,s/da) );
   // float b = UVW.y/w; b = pow( 2., -2.*pow(b,s/db) );
   // float c = UVW.z/w; c = pow( 2., -2.*pow(c,s/dc) );
   // return max(max(a,b),c);

   // variable width lines, but not directly proportional
   const float w = .015; // width
   const float s = .4; // hardness
   float da = length(vec2(dFdx(UVW.x),dFdy(UVW.x)));
   float db = length(vec2(dFdx(UVW.y),dFdy(UVW.y)));
   float dc = length(vec2(dFdx(UVW.z),dFdy(UVW.z)));
   float a = UVW.x/(10.*sqrt(da)*w); a = pow( 2., -2.*pow(a,s/sqrt(da)) );
   float b = UVW.y/(10.*sqrt(db)*w); b = pow( 2., -2.*pow(b,s/sqrt(db)) );
   float c = UVW.z/(10.*sqrt(dc)*w); c = pow( 2., -2.*pow(c,s/sqrt(dc)) );
   return max(max(a,b),c);
}

float getEdgeFactor(vec2 UV)
{
   const float w = .015; // width
   const float s = .4; // hardness
   float da = length(vec2(dFdx(UV.x),dFdy(UV.x)));
   float db = length(vec2(dFdx(UV.y),dFdy(UV.y)));
   float a = (.5-abs(.5-UV.x))/(10.*sqrt(da)*w); a = pow( 2., -2.*pow(a,s/sqrt(da)) );
   float b = (.5-abs(.5-UV.y))/(10.*sqrt(db)*w); b = pow( 2., -2.*pow(b,s/sqrt(db)) );
   return max(a,b);
}

float check( vec2 uv )
{
   float c;
   const float frequency = 256.;
   vec2 st = frequency * uv;
   vec2 w = fwidth( st );
   vec2 fuzz = w * 2.;
   float f = max( fuzz.s, fuzz.t );
   vec2 q = fract( st );

   if( f < .5 )
   {
      vec2 p = smoothstep( vec2(.5), fuzz+vec2(.5), q) +
         (1. - smoothstep(vec2(0.), fuzz, q));

      c = mix( .4, .6, p.x * p.y + (1.-p.x) * (1.-p.y));
      c = mix( c, .5, smoothstep( .125, .5, f ));
   }
   else
   {
      c = .5;
   }

   return c;

   // float c = mod( floor(P.x) + floor(P.z), 2 );
   // c = .5 - c*.125;
   // return c;
}


)";


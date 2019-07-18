// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader PARAM_SURFACE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_modelView", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_normal", GLData::Vector3Float},
        {"a_coord", GLData::Vector2Float},
    },

    // source
    POLYSCOPE_GLSL(150,
      uniform mat4 u_modelView;
      uniform mat4 u_projMatrix;
      in vec3 a_position;
      in vec3 a_normal;
      in vec2 a_coord;
      out vec3 Normal;
      out vec2 Coord;

      void main()
      {
          Normal = mat3(u_modelView) * a_normal;
          Coord = a_coord;
          gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
      }
    )
};

static const FragShader PARAM_CHECKER_SURFACE_FRAG_SHADER = { 
  
  
    // uniforms 
    {
        {"u_modLen", GLData::Float},
        {"u_color1", GLData::Vector3Float},
        {"u_color2", GLData::Vector3Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform vec3 u_color1;
      uniform vec3 u_color2;
      uniform float u_modLen;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;

      in vec3 Normal;
      in vec2 Coord;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      void main()
      {
        // Apply the checkerboard effect
        float mX = mod(Coord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(Coord.y, 2.0 * u_modLen) / u_modLen - 1.f;

        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        // TODO do some clever screen space derivative thing to prevent aliasing

        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;

        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(u_color1, u_color2, s);


        outputF = lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};



static const FragShader PARAM_GRID_SURFACE_FRAG_SHADER = { 
  
  
    // uniforms 
    {
        {"u_modLen", GLData::Float},
        {"u_gridLineColor", GLData::Vector3Float},
        {"u_gridBackgroundColor", GLData::Vector3Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform vec3 u_gridLineColor;
      uniform vec3 u_gridBackgroundColor;
      uniform float u_modLen;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;

      in vec3 Normal;
      in vec2 Coord;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      void main()
      {
        // Apply the checkerboard effect
        float mX = mod(Coord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(Coord.y, 2.0 * u_modLen) / u_modLen - 1.f;


        float minD = min(min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        
        float width = 0.05;
        float slopeWidthPix = 5.;

        vec2 fw = fwidth(Coord);
        float scale = max(fw.x, fw.y);
        float pWidth = slopeWidthPix * scale;

        float s = smoothstep(width, width + pWidth, minD);
        vec3 outColor = mix(u_gridLineColor, u_gridBackgroundColor, s);


        outputF = lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};


static const FragShader PARAM_LOCAL_RAD_SURFACE_FRAG_SHADER = { 
  
  
    // uniforms 
    {
        {"u_modLen", GLData::Float},
        {"u_angle", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_colormap", 1}
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform float u_modLen;
      uniform float u_angle;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler1D t_colormap;

      in vec3 Normal;
      in vec2 Coord;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      void main()
      {

        // Get the color at this point
        float pi = 3.14159265359;
        float angle = atan(Coord.y, Coord.x) / (2. * pi) + 0.5; // in [0,1]
        float shiftedAngle = mod(angle + u_angle/(2. * pi), 1.);
        vec3 color = texture(t_colormap, shiftedAngle).rgb;
        vec3 colorDark = color * .5;

        // Apply the checkerboard effect (vert similar to rectangular checker
        float mX = mod(length(Coord), 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float minD = min(abs(mX), 1.0 - abs(mX)) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        float v = mX; // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(color, colorDark, s);

        outputF = lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};

static const FragShader PARAM_LOCAL_CHECKER_SURFACE_FRAG_SHADER = { 
  
  
    // uniforms 
    {
        {"u_modLen", GLData::Float},
        {"u_angle", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_colormap", 1}
    },
    
    // output location
    "outputF",
    
    // source 
    POLYSCOPE_GLSL(150,
      uniform float u_modLen;
      uniform float u_angle;

      uniform sampler2D t_mat_r;
      uniform sampler2D t_mat_g;
      uniform sampler2D t_mat_b;
      uniform sampler1D t_colormap;

      in vec3 Normal;
      in vec2 Coord;
      out vec4 outputF;

      // Forward declarations of methods from <shaders/common.h>
      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);

      void main()
      {

        // Rotate coords
        float cosT = cos(u_angle);
        float sinT = sin(u_angle);
        vec2 rotCoord = vec2(cosT * Coord.x - sinT * Coord.y, sinT * Coord.x + cosT * Coord.y);
          
        // Get the color at this point
        float pi = 3.14159265359;
        float angle = atan(rotCoord.y, rotCoord.x) / (2. * pi) + 0.5; // in [0,1]
        vec3 color = texture(t_colormap, angle).rgb;
        vec3 colorDark = color * .5;

        // Apply the checkerboard effect (copied from checker above)
        float mX = mod(rotCoord.x, 2.0 * u_modLen) / u_modLen - 1.f; // in [-1, 1]
        float mY = mod(rotCoord.y, 2.0 * u_modLen) / u_modLen - 1.f;
        float minD = min( min(abs(mX), 1.0 - abs(mX)), min(abs(mY), 1.0 - abs(mY))) * 2.; // rect distace from flipping sign in [0,1]
        float p = 6;
        float minDSmooth = pow(minD, 1. / p);
        float v = (mX * mY); // in [-1, 1], color switches at 0
        float adjV = sign(v) * minDSmooth;
        float s = smoothstep(-1.f, 1.f, adjV);

        vec3 outColor = mix(color, colorDark, s);

        outputF = lightSurfaceMat(Normal, outColor, t_mat_r, t_mat_g, t_mat_b);
      }

    )
};

// clang-format on

} // namespace gl
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/shaders/texture_draw_shaders.h"

// clang-format off

namespace polyscope {
namespace render{
namespace backend_openGL3_glfw {

const ShaderStageSpecification  TEXTURE_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    { },

    // attributes
    {
        {"a_position", DataType::Vector3Float},
    },

    // textures
    {},
    
    // source
R"(
      ${ GLSL_VERSION }$
      in vec3 a_position;
      out vec2 tCoord;

      void main()
      {
          tCoord = (a_position.xy+vec2(1.0,1.0))/2.0;
          gl_Position = vec4(a_position,1.);
      }
)"
};

const ShaderStageSpecification  SPHEREBG_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    { 
       {"u_viewMatrix", DataType::Matrix44Float},
       {"u_projMatrix", DataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", DataType::Vector4Float},
    },

    // textures
    {},
    
    // source
R"(
      ${ GLSL_VERSION }$

      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      in vec4 a_position;
      out vec3 viewDir;

      void main()
      {
          vec4 viewPos4 = u_viewMatrix * a_position;
          //viewDir = normalize(vec3(viewPos4));
          viewDir = a_position.xyz;
          vec4 projPos = u_projMatrix * viewPos4;
          projPos.z = 1. * projPos.w; // set depth to max value
          gl_Position = projPos;
      }
)"
};

const ShaderStageSpecification PLAIN_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        outputF = vec4(texture(t_image, tCoord).rgba);
      }
)"
};

const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_mapDot", DataType::Vector3Float},
    }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform vec3 u_mapDot;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        float sampleVal = dot(u_mapDot, texture(t_image, tCoord).rgb);
        outputF = vec4(sampleVal, 0., 0., 1.);
      }
)"
};

const ShaderStageSpecification MAP3_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_scale", DataType::Vector3Float},
        {"u_shift", DataType::Vector3Float},
    }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform vec3 u_scale;
      uniform vec3 u_shift;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        vec3 val = texture(t_image, tCoord).rgb;
        vec3 mapped = (val + u_shift) * u_scale;
        outputF = vec4(mapped, 1.);
      }
)"
};

const ShaderStageSpecification SPHEREBG_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec3 viewDir;
      uniform sampler2D t_image;
      layout(location = 0) out vec4 outputF;

      vec2 sphericalTexCoords(vec3 v);

      void main()
      {
        vec3 viewDirN = normalize(viewDir);
        vec2 sampleCoords = sphericalTexCoords(viewDirN);
        vec3 val = texture(t_image, sampleCoords).rgb;
        //val = vec3(sampleCoords.y, 0.0, 0.0);
        outputF = vec4(val, 1.);
      }
)"
};


const ShaderStageSpecification COMPOSITE_PEEL = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { {"t_image", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      layout(location = 0) out vec4 outputF;

      void main()
      {
        vec4 val = texture(t_image, tCoord);
        val.rgb = val.rgb * val.a; // premultiply
        outputF = val;
      }
)"
};

const ShaderStageSpecification DEPTH_COPY = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { {"t_depth", 2} },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_depth;
      //layout(location = 0) out vec4 outputF;

      void main()
      {
        float depth = texture(t_depth, tCoord).r;
        gl_FragDepth = depth;
        //outputF = val;
      }
)"
};

const ShaderStageSpecification SCALAR_TEXTURE_COLORMAP = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_scalar", 2},
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_scalar;
      layout(location = 0) out vec4 outputF;
        
      ${ FRAG_DECLARATIONS }$

      void main()
      {
        float shadeValue = texture(t_scalar, tCoord).r;

        ${ GENERATE_SHADE_COLOR }$

        outputF = vec4(albedoColor, 1.);
      }
)"
};

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

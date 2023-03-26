// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/opengl/shaders/texture_draw_shaders.h"
#include "polyscope/render/engine.h"

// clang-format off

namespace polyscope {
namespace render{
namespace backend_openGL3_glfw {

// this uses the default openGL convention of origin in the lower left
const ShaderStageSpecification  TEXTURE_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    { },

    // attributes
    {
        {"a_position", RenderDataType::Vector3Float},
    },

    // textures
    {},
    
    // source
R"(
      ${ GLSL_VERSION }$
      in vec3 a_position;
      out vec2 tCoord;
      
      ${ VERT_DECLARATIONS }$

      void main()
      {
          tCoord = (a_position.xy+vec2(1.0,1.0))/2.0;
          ${ TCOORD_ADJUST }$

          vec4 position = vec4(a_position,1.0);
          ${ POSITION_ADJUST }$

          gl_Position = position;
      }
)"
};

const ShaderStageSpecification  SPHEREBG_DRAW_VERT_SHADER =  {

    // stage
    ShaderStageType::Vertex,
    
    // uniforms
    { 
       {"u_viewMatrix", RenderDataType::Matrix44Float},
       {"u_projMatrix", RenderDataType::Matrix44Float},
    },

    // attributes
    {
        {"a_position", RenderDataType::Vector4Float},
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
      
      ${ FRAG_DECLARATIONS }$

      void main()
      {
        vec4 textureOut = texture(t_image, tCoord).rgba;

        ${ TEXTURE_OUT_ADJUST }$ 

        outputF = textureOut;
      }
)"
};

const ShaderStageSpecification PLAIN_RENDERIMAGE_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_projMatrix", RenderDataType::Matrix44Float},
        {"u_invProjMatrix", RenderDataType::Matrix44Float},
        {"u_viewport", RenderDataType::Vector4Float},
        {"u_transparency", RenderDataType::Float},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_depth", 2},
      {"t_normal", 2},
    },
    
    // source 
R"(

  ${ GLSL_VERSION }$
  uniform mat4 u_projMatrix; 
  uniform mat4 u_invProjMatrix;
  uniform vec4 u_viewport;
  uniform float u_transparency;

  in vec2 tCoord;
  uniform sampler2D t_depth;
  uniform sampler2D t_normal;
  layout(location = 0) out vec4 outputF;
    
  float LARGE_FLOAT();
  vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
  float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);

  ${ FRAG_DECLARATIONS }$

  void main() {

    // Fetch values from texture
    float depth = texture(t_depth, tCoord).r;
    vec3 normal = normalize(texture(t_normal, tCoord).rgb);

    if(depth > LARGE_FLOAT()) {
      discard;
    }

    // Set the depth of the fragment from the stored texture data
    // TODO: this a wasteful way to convert ray depth to gl_FragDepth, I am sure it can be done with much less arithmetic... figure it out 
    // Build a ray corresponding to this fragment
    vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
    vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);
    // (source is implicit at origin)
    vec3 viewPos = normalize(viewRay) * depth;
    float fragdepth = fragDepthFromView(u_projMatrix, depthRange, viewPos);
    gl_FragDepth = fragdepth;

    
    // Shading
    ${ GENERATE_SHADE_VALUE }$
    ${ GENERATE_SHADE_COLOR }$

    // Lighting
    vec3 shadeNormal = normal;
    ${ GENERATE_LIT_COLOR }$

     // Set alpha
    float alphaOut = u_transparency;
    ${ GENERATE_ALPHA }$

    // Write output
    outputF = vec4(litColor, alphaOut);

  }
)"
};

const ShaderStageSpecification DOT3_TEXTURE_DRAW_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_mapDot", RenderDataType::Vector3Float},
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
        {"u_scale", RenderDataType::Vector3Float},
        {"u_shift", RenderDataType::Vector3Float},
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

const ShaderStageSpecification DEPTH_TO_MASK = {
  // writes 0./1. mask to red channel
    
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
      layout(location = 0) out vec4 outputF;

      void main()
      {
        float depth = texture(t_depth, tCoord).r;
        if(depth < 1.) {
          outputF = vec4(1., 0., 0., 1.);
        } else {
          outputF = vec4(0., 0., 0., 1.);
        }
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

        vec4 textureOut = vec4(albedoColor, 1.);

        ${ TEXTURE_OUT_ADJUST }$ 

        outputF = textureOut;
      }
)"
};


const ShaderStageSpecification BLUR_RGB = {
  // Separable gaussian blur. Run twice between a pair of buffers, once with horizontal=0, and once with horizontal=1.
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
      {"u_horizontal", RenderDataType::Int},
    }, 

    // attributes
    { },
    
    // textures 
    { 
      {"t_image", 2},
    },
    
    // source 
R"(
      ${ GLSL_VERSION }$

      in vec2 tCoord;
      uniform sampler2D t_image;
      uniform int u_horizontal;
      uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216); // gaussian blur weights
      layout(location = 0) out vec4 outputF;

      void main()
      {
        vec2 texScale = 1.0 / textureSize(t_image, 0);
        vec3 valCenter = texture(t_image, tCoord).rgb;
        vec3 val = valCenter * weight[0];
        if(u_horizontal == 1) {
            for(int i = 1; i < 5; ++i) {
                val += texture(t_image, tCoord + vec2(texScale.x * i, 0.0)).rgb * weight[i];
                val += texture(t_image, tCoord - vec2(texScale.x * i, 0.0)).rgb * weight[i];
            }
        }
        else {
            for(int i = 1; i < 5; ++i) {
                val += texture(t_image, tCoord + vec2(0.0, texScale.y * i)).rgb * weight[i];
                val += texture(t_image, tCoord - vec2(0.0, texScale.y * i)).rgb * weight[i];
            }
        }

        outputF = vec4(val, 1.);
      }
)"
};

const ShaderReplacementRule TEXTURE_ORIGIN_UPPERLEFT (
    /* rule name */ "TEXTURE_ORIGIN_UPPERLEFT",
    { /* replacement sources */
      {"TCOORD_ADJUST", R"(
        tCoord = vec2(tCoord.x, 1. - tCoord.y);
      )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule TEXTURE_ORIGIN_LOWERLEFT (
    // nothing needs to be done, this is already the default
    
    /* rule name */ "TEXTURE_ORIGIN_UPPERLEFT",
    /* replacement sources */ {}, 
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {}
);

const ShaderReplacementRule TEXTURE_SET_TRANSPARENCY(
    /* rule name */ "TEXTURE_SET_TRANSPARENCY",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform float u_transparency;
        )" },
      {"TEXTURE_OUT_ADJUST", R"(
        textureOut = vec4(textureOut.rgb, textureOut.a * u_transparency);
      )"}
    },
    /* uniforms */ {
        {"u_transparency", RenderDataType::Float},
    },
    /* attributes */ {},
    /* textures */ {}
);

// input: vec2 tcoord, 2d --> 3d texture t_color
// output: vec3 albedoColor
const ShaderReplacementRule TEXTURE_SHADE_COLOR(
    /* rule name */ "TEXTURE_SHADE_COLOR",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler2D t_color;
        )" },
      {"GENERATE_SHADE_COLOR", R"(
        vec3 sampledTextureColor = texture(t_color, tCoord).rgb;
        vec3 albedoColor = sampledTextureColor;
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {
      {"t_color", 2},
    }
);

// input: vec2 tcoord, 1d --> 3d texture t_scalar
// output: vec3 albedoColor
const ShaderReplacementRule TEXTURE_PROPAGATE_VALUE(
    /* rule name */ "TEXTURE_PROPAGATE_VALUE",
    { /* replacement sources */
      {"FRAG_DECLARATIONS", R"(
          uniform sampler2D t_scalar;
        )" },
      {"GENERATE_SHADE_VALUE", R"(
        float shadeValue = texture(t_scalar, tCoord).r;
        )"}
    },
    /* uniforms */ {},
    /* attributes */ {},
    /* textures */ {
      {"t_scalar", 2},
    }
);

const ShaderReplacementRule TEXTURE_BILLBOARD_FROM_UNIFORMS(
    /* rule name */ "TEXTURE_BILLBOARD_FROM_UNIFORMS",
    { /* replacement sources */
      {"VERT_DECLARATIONS", R"(
          uniform mat4 u_modelView;
          uniform mat4 u_projMatrix;
          uniform vec3 u_billboardCenter;
          uniform vec3 u_billboardUp;
          uniform vec3 u_billboardRight;
        )" },
      {"POSITION_ADJUST", R"(
        vec3 positionWorld = position.x * u_billboardRight + position.y * u_billboardUp + u_billboardCenter;
        position = u_projMatrix * u_modelView * vec4(positionWorld, 1.0);
      )"}
    },
    /* uniforms */ {
      {"u_modelView", RenderDataType::Matrix44Float},
      {"u_projMatrix", RenderDataType::Matrix44Float},
      {"u_billboardCenter", RenderDataType::Vector3Float},
      {"u_billboardUp", RenderDataType::Vector3Float},
      {"u_billboardRight", RenderDataType::Vector3Float}
    },
    /* attributes */ {},
    /* textures */ {}
);

// clang-format on

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

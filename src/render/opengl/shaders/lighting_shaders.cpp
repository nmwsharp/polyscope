// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_shaders.h"

// clang-format off

namespace polyscope {
namespace render{

const ShaderStageSpecification MAP_LIGHT_FRAG_SHADER = {
    
    // stage
    ShaderStageType::Fragment,
    
    // uniforms
    { 
        {"u_exposure", DataType::Float},
        {"u_gamma", DataType::Float},
        {"u_whiteLevel", DataType::Float},
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
      uniform float u_whiteLevel;
      uniform float u_gamma;
      layout (location = 0) out vec4 outputVal;

      void main() {

        vec4 color4 = texture(t_image, tCoord);
        vec3 color = color4.rgb;
        float alpha = color4.a;

				// "lighting"
				color = color * u_exposure;

        // tonemapping (extended Reinhard)
				vec3 num = color * (1.0f + (color / vec3(u_whiteLevel * u_whiteLevel)));
				vec3 den = (1.0f + color);
				color = num / den;
        
        // gamma correction
				color = pow(color, vec3(1.0/u_gamma));  
       
        outputVal = vec4(color, alpha);
    }  
    )
};


// clang-format on

} // namespace render
} // namespace polyscope

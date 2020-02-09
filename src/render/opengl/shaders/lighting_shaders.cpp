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


// clang-format on

} // namespace render
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <cstdlib>
#include <string>
#include <vector>

// Make syntax  nicer like this, but we lose line numbers in GL debug output
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

namespace polyscope {
namespace gl {

// Enum for openGL data types
enum class GLData { Vector2Float, Vector3Float, Vector4Float, Matrix44Float, Float, Int, UInt, Index };

// Types to encapsulate uniform and attribute variables in shaders
struct ShaderUniform {
  const std::string name;
  const GLData type;
};
struct ShaderAttribute {
  ShaderAttribute(std::string name_, GLData type_) : name(name_), type(type_), arrayCount(1) {}
  ShaderAttribute(std::string name_, GLData type_, int arrayCount_)
      : name(name_), type(type_), arrayCount(arrayCount_) {}
  const std::string name;
  const GLData type;
  const int arrayCount; // number of times this element is repeated in an array
};
struct ShaderTexture {
  const std::string name;
  const int dim;
};


// Types which represents shaders and the values they require
struct VertShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
struct TessShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
struct EvalShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
struct GeomShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
struct FragShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::vector<ShaderTexture> textures;
  const std::string outputLoc;
  const std::string src;
};

} // namespace gl
} // namespace polyscope

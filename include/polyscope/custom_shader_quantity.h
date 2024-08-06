// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/types.h"

#include <exception>
#include <map>
#include <stdexcept>
#include <utility>

namespace polyscope {

// Custom quantities allow users to write their own shaders

class CustomShaderQuantity {
public:
  CustomShaderQuantity(const std::string& programText);

  // Build the ImGUI UI
  void buildUI();
  void buildEditorUI();

  // Add rules to rendering programs for scalars
  // std::vector<std::string> addColorRules(std::vector<std::string> rules);

  // Set uniforms in rendering programs for scalars
  // void setColorUniforms(render::ShaderProgram& p);

  // === Members

  // === Custom Shader Attributes
  template <typename T>
  void makeAttributeAvailable(std::string quantityName, std::string shaderAttributeName = "");

  // === Custom Shader Uniforms
  template <typename T>
  void addUniform(std::string name, T initVal);

  template <typename T>
  void setUniform(std::string name, T newVal);

  template <typename T>
  T& getUniform(std::string name);


  // === Get/set visualization parameters

  // === ~DANGER~ experimental/unsupported functions

protected:
  std::string programText;

  struct CustomShaderAttributeEntry {
    std::string quantityName;
    std::string attributeName;
    bool isResolved;
    std::shared_ptr<render::AttributeBuffer> attributeBuffer;
    GenericWeakHandle managedBufferWeakHandle;
  };
  std::vector<CustomShaderAttributeEntry> attributes;

  // === Visualization parameters
  std::shared_ptr<render::ShaderProgram> program;

  // Parameters
  void finishBuildingProgram();
  void markProgramStale();
  void setProgramAttributes();
  void setProgramUniforms();


  void resolveAttributes();
  virtual void resolveAttribute(CustomShaderAttributeEntry& entry) = 0;
  virtual void setProgramStructureAttributes();

  // clang-format off

  // Uniform maps
  std::map<std::string, float>      uniformMap_float;
  std::map<std::string, glm::vec2>  uniformMap_vec2;
  std::map<std::string, glm::vec3>  uniformMap_vec3;
  std::map<std::string, glm::vec4>  uniformMap_vec4;
  std::map<std::string, int32_t>    uniformMap_int32;
  
  // Attribute maps
  std::map<std::string, std::shared_ptr<render::AttributeBuffer>>    attributeMap_float;
  std::map<std::string, std::shared_ptr<render::AttributeBuffer>>    attributeMap_vec2;
  std::map<std::string, std::shared_ptr<render::AttributeBuffer>>    attributeMap_vec3;
  std::map<std::string, std::shared_ptr<render::AttributeBuffer>>    attributeMap_vec4;
  std::map<std::string, std::shared_ptr<render::AttributeBuffer>>    attributeMap_int32;

  // clang-format on
};

// Custom exception type
struct CustomShaderError : public std::runtime_error {
  using std::runtime_error::runtime_error; // constructor inheritance
};

} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/custom_shader_quantity.h"

namespace polyscope {

void CustomShaderQuantity::finishBuildingProgram() {
  setProgramAttributes();
  setProgramUniforms();
  program->validateData();
}


void CustomShaderQuantity::resolveAttributes() {
  for (CustomShaderAttributeEntry& entry : attributes) {
    if (!entry.managedBufferWeakHandle.isValid()) {
      entry.isResolved = false;
      entry.attributeBuffer.reset();
    
  }

  for (CustomShaderAttributeEntry& entry : attributes) {
    if (!entry.isResolved) {
      resolveAttribute(entry);
    }
  }
}


void CustomShaderQuantity::setProgramAttributes() {
  // clang-format off
  for (auto const& entry : attributeMap_float) { if (program->hasAttribute(entry.first)) { program->setAttribute(entry.first, entry.second); } }
  for (auto const& entry : attributeMap_vec2 ) { if (program->hasAttribute(entry.first)) { program->setAttribute(entry.first, entry.second); } }
  for (auto const& entry : attributeMap_vec3 ) { if (program->hasAttribute(entry.first)) { program->setAttribute(entry.first, entry.second); } }
  for (auto const& entry : attributeMap_vec4 ) { if (program->hasAttribute(entry.first)) { program->setAttribute(entry.first, entry.second); } }
  for (auto const& entry : attributeMap_int32) { if (program->hasAttribute(entry.first)) { program->setAttribute(entry.first, entry.second); } }
  // clang-format on
}

void CustomShaderQuantity::setProgramStructureAttributes() {
  // pass
  // subclasses can override this
}

void CustomShaderQuantity::setProgramUniforms() {
  // clang-format off
  for(auto const& entry : uniformMap_float) { if(program->hasUniform(entry.first)) { program->setUniform(entry.first, entry.second); } }
  for(auto const& entry : uniformMap_vec2 ) { if(program->hasUniform(entry.first)) { program->setUniform(entry.first, entry.second); } }
  for(auto const& entry : uniformMap_vec3 ) { if(program->hasUniform(entry.first)) { program->setUniform(entry.first, entry.second); } }
  for(auto const& entry : uniformMap_vec4 ) { if(program->hasUniform(entry.first)) { program->setUniform(entry.first, entry.second); } }
  for(auto const& entry : uniformMap_int32) { if(program->hasUniform(entry.first)) { program->setUniform(entry.first, entry.second); } }
  // clang-format on
}

// clang-format off

// addUniform()
template <> void CustomShaderQuantity::addUniform<float     >(std::string name, float     initVal) { uniformMap_float.emplace(name, initVal); }
template <> void CustomShaderQuantity::addUniform<glm::vec2 >(std::string name, glm::vec2 initVal) { uniformMap_vec2 .emplace(name, initVal); }
template <> void CustomShaderQuantity::addUniform<glm::vec3 >(std::string name, glm::vec3 initVal) { uniformMap_vec3 .emplace(name, initVal); }
template <> void CustomShaderQuantity::addUniform<glm::vec4 >(std::string name, glm::vec4 initVal) { uniformMap_vec4 .emplace(name, initVal); }
template <> void CustomShaderQuantity::addUniform<int32_t   >(std::string name, int32_t   initVal) { uniformMap_int32.emplace(name, initVal); }

// setUniform()
template <> void CustomShaderQuantity::setUniform<float     >(std::string name, float     newVal) { uniformMap_float[name] = newVal; }
template <> void CustomShaderQuantity::setUniform<glm::vec2 >(std::string name, glm::vec2 newVal) { uniformMap_vec2 [name] = newVal; }
template <> void CustomShaderQuantity::setUniform<glm::vec3 >(std::string name, glm::vec3 newVal) { uniformMap_vec3 [name] = newVal; }
template <> void CustomShaderQuantity::setUniform<glm::vec4 >(std::string name, glm::vec4 newVal) { uniformMap_vec4 [name] = newVal; }
template <> void CustomShaderQuantity::setUniform<int32_t   >(std::string name, int32_t   newVal) { uniformMap_int32[name] = newVal; }

// getUniform()
template <> float    & CustomShaderQuantity::getUniform<float     >(std::string name) {if(uniformMap_float.find(name) == uniformMap_float.end()) { error("no uniform with that type of name " + name); } return uniformMap_float[name]; }
template <> glm::vec2& CustomShaderQuantity::getUniform<glm::vec2 >(std::string name) {if(uniformMap_vec2 .find(name) == uniformMap_vec2 .end()) { error("no uniform with that type of name " + name); } return uniformMap_vec2 [name]; }
template <> glm::vec3& CustomShaderQuantity::getUniform<glm::vec3 >(std::string name) {if(uniformMap_vec3 .find(name) == uniformMap_vec3 .end()) { error("no uniform with that type of name " + name); } return uniformMap_vec3 [name]; }
template <> glm::vec4& CustomShaderQuantity::getUniform<glm::vec4 >(std::string name) {if(uniformMap_vec4 .find(name) == uniformMap_vec4 .end()) { error("no uniform with that type of name " + name); } return uniformMap_vec4 [name]; }
template <> int32_t  & CustomShaderQuantity::getUniform<int32_t   >(std::string name) {if(uniformMap_int32.find(name) == uniformMap_int32.end()) { error("no uniform with that type of name " + name); } return uniformMap_int32[name]; }

// clang-format on

} // namespace polyscope

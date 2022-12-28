
// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/render/templated_buffers.h"

namespace polyscope {
namespace render {

// == Generate Buffer

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<float>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Float, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<double>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Float, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec2>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector2Float, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec3>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3Float, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec4>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector4Float, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<size_t>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::UInt, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<uint32_t>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::UInt, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<int32_t>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Int, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec2>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector2UInt, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec3>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3UInt, arrayCount);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec4>(Engine* engine, int arrayCount) {
  return engine->generateAttributeBuffer(RenderDataType::Vector4UInt, arrayCount);
}

// == Get buffer data


} // namespace render
} // namespace polyscope

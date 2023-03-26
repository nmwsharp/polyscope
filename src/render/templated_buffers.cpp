// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include <vector>

#include "polyscope/render/templated_buffers.h"

namespace polyscope {
namespace render {

// == Generate Buffer

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<float>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Float);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<double>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Float);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec2>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector2Float);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec3>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3Float);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<std::array<glm::vec3, 2>>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3Float, 2);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<std::array<glm::vec3, 3>>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3Float, 3);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<std::array<glm::vec3, 4>>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3Float, 4);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::vec4>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector4Float);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<size_t>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::UInt);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<uint32_t>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::UInt);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<int32_t>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Int);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec2>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector2UInt);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec3>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector3UInt);
}

template <>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer<glm::uvec4>(Engine* engine) {
  return engine->generateAttributeBuffer(RenderDataType::Vector4UInt);
}

// == Get buffer data at a single location

template <>
float getAttributeBufferData<float>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_float(ind);
}

template <>
double getAttributeBufferData<double>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_double(ind);
}

template <>
glm::vec2 getAttributeBufferData<glm::vec2>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_vec2(ind);
}

template <>
glm::vec3 getAttributeBufferData<glm::vec3>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_vec3(ind);
}

template <>
std::array<glm::vec3, 2> getAttributeBufferData<std::array<glm::vec3, 2>>(AttributeBuffer& buff, size_t ind) {
  std::array<glm::vec3, 2> out;
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(2 * ind, 2);
  for (size_t i = 0; i < 2; i++) out[i] = fetch[i];
  return out;
}

template <>
std::array<glm::vec3, 3> getAttributeBufferData<std::array<glm::vec3, 3>>(AttributeBuffer& buff, size_t ind) {
  std::array<glm::vec3, 3> out;
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(3 * ind, 3);
  for (size_t i = 0; i < 3; i++) out[i] = fetch[i];
  return out;
}

template <>
std::array<glm::vec3, 4> getAttributeBufferData<std::array<glm::vec3, 4>>(AttributeBuffer& buff, size_t ind) {
  std::array<glm::vec3, 4> out;
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(4 * ind, 4);
  for (size_t i = 0; i < 4; i++) out[i] = fetch[i];
  return out;
}

template <>
glm::vec4 getAttributeBufferData<glm::vec4>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_vec4(ind);
}

template <>
size_t getAttributeBufferData<size_t>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_uint32(ind);
}

template <>
uint32_t getAttributeBufferData<uint32_t>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_uint32(ind);
}

template <>
int32_t getAttributeBufferData<int32_t>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_int(ind);
}

template <>
glm::uvec2 getAttributeBufferData<glm::uvec2>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_uvec2(ind);
}

template <>
glm::uvec3 getAttributeBufferData<glm::uvec3>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_uvec3(ind);
}

template <>
glm::uvec4 getAttributeBufferData<glm::uvec4>(AttributeBuffer& buff, size_t ind) {
  return buff.getData_uvec4(ind);
}

// == Get buffer data at a range of locations

template <>
std::vector<float> getAttributeBufferDataRange<float>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_float(ind, count);
}

template <>
std::vector<double> getAttributeBufferDataRange<double>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_double(ind, count);
}

template <>
std::vector<glm::vec2> getAttributeBufferDataRange<glm::vec2>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_vec2(ind, count);
}

template <>
std::vector<glm::vec3> getAttributeBufferDataRange<glm::vec3>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_vec3(ind, count);
}

template <>
std::vector<std::array<glm::vec3, 2>> getAttributeBufferDataRange<std::array<glm::vec3, 2>>(AttributeBuffer& buff,
                                                                                            size_t ind, size_t count) {
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(2 * ind, 2 * count);
  std::vector<std::array<glm::vec3, 2>> out(count);
  for (size_t i = 0; i < count; i++) {
    for (size_t j = 0; j < 2; j++) {
      out[i][j] = fetch[2 * i + j];
    }
  }
  return out;
}

template <>
std::vector<std::array<glm::vec3, 3>> getAttributeBufferDataRange<std::array<glm::vec3, 3>>(AttributeBuffer& buff,
                                                                                            size_t ind, size_t count) {
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(3 * ind, 3 * count);
  std::vector<std::array<glm::vec3, 3>> out(count);
  for (size_t i = 0; i < count; i++) {
    for (size_t j = 0; j < 3; j++) {
      out[i][j] = fetch[3 * i + j];
    }
  }
  return out;
}

template <>
std::vector<std::array<glm::vec3, 4>> getAttributeBufferDataRange<std::array<glm::vec3, 4>>(AttributeBuffer& buff,
                                                                                            size_t ind, size_t count) {
  std::vector<glm::vec3> fetch = buff.getDataRange_vec3(4 * ind, 4 * count);
  std::vector<std::array<glm::vec3, 4>> out(count);
  for (size_t i = 0; i < count; i++) {
    for (size_t j = 0; j < 4; j++) {
      out[i][j] = fetch[4 * i + j];
    }
  }
  return out;
}

template <>
std::vector<glm::vec4> getAttributeBufferDataRange<glm::vec4>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_vec4(ind, count);
}

template <>
std::vector<size_t> getAttributeBufferDataRange<size_t>(AttributeBuffer& buff, size_t ind, size_t count) {
  std::vector<uint32_t> uint32Vals = buff.getDataRange_uint32(ind, count);
  std::vector<size_t> sizetVals(count);
  for (size_t i = 0; i < count; i++) {
    sizetVals[i] = static_cast<size_t>(uint32Vals[i]);
  }
  return sizetVals;
}

template <>
std::vector<uint32_t> getAttributeBufferDataRange<uint32_t>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_uint32(ind, count);
}

template <>
std::vector<int32_t> getAttributeBufferDataRange<int32_t>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_int(ind, count);
}

template <>
std::vector<glm::uvec2> getAttributeBufferDataRange<glm::uvec2>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_uvec2(ind, count);
}

template <>
std::vector<glm::uvec3> getAttributeBufferDataRange<glm::uvec3>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_uvec3(ind, count);
}

template <>
std::vector<glm::uvec4> getAttributeBufferDataRange<glm::uvec4>(AttributeBuffer& buff, size_t ind, size_t count) {
  return buff.getDataRange_uvec4(ind, count);
}

} // namespace render
} // namespace polyscope

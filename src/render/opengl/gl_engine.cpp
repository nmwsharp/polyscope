// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "backends/imgui_impl_opengl3.h"
#include "polyscope/render/engine.h"

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED
#include "polyscope/render/opengl/gl_engine.h"

#include "polyscope/messages.h"
#include "polyscope/options.h"
#include "polyscope/polyscope.h"
#include "polyscope/utilities.h"

#include "polyscope/render/shader_builder.h"

// all the shaders
#include "polyscope/render/opengl/shaders/common.h"
#include "polyscope/render/opengl/shaders/cylinder_shaders.h"
#include "polyscope/render/opengl/shaders/gizmo_shaders.h"
#include "polyscope/render/opengl/shaders/ground_plane_shaders.h"
#include "polyscope/render/opengl/shaders/histogram_shaders.h"
#include "polyscope/render/opengl/shaders/lighting_shaders.h"
#include "polyscope/render/opengl/shaders/ribbon_shaders.h"
#include "polyscope/render/opengl/shaders/rules.h"
#include "polyscope/render/opengl/shaders/sphere_shaders.h"
#include "polyscope/render/opengl/shaders/surface_mesh_shaders.h"
#include "polyscope/render/opengl/shaders/texture_draw_shaders.h"
#include "polyscope/render/opengl/shaders/vector_shaders.h"
#include "polyscope/render/opengl/shaders/volume_mesh_shaders.h"

#include "stb_image.h"

#include <set>

namespace polyscope {
namespace render {

namespace backend_openGL3_glfw {

GLEngine* glEngine = nullptr; // alias for global engine pointer

void initializeRenderEngine() {
  glEngine = new GLEngine();
  engine = glEngine;
  glEngine->initialize();
  engine->allocateGlobalBuffersAndPrograms();
}

// == Map enums to native values

// clang-format off

inline GLenum internalFormat(const TextureFormat& x) {
  switch (x) {
    case TextureFormat::RGB8:       return GL_RGB8;
    case TextureFormat::RGBA8:      return GL_RGBA8;
    case TextureFormat::RG16F:      return GL_RG16F;
    case TextureFormat::RGB16F:     return GL_RGB16F;
    case TextureFormat::RGBA16F:    return GL_RGBA16F;
    case TextureFormat::R32F:       return GL_R32F;
    case TextureFormat::R16F:       return GL_R16F;
    case TextureFormat::RGB32F:     return GL_RGBA32F;
    case TextureFormat::RGBA32F:    return GL_RGBA32F;
    case TextureFormat::DEPTH24:    return GL_DEPTH_COMPONENT24;
  }
  exception("bad enum");
  return GL_RGB8;
}

inline GLenum formatF(const TextureFormat& x) {
  switch (x) {
    case TextureFormat::RGB8:       return GL_RGB;
    case TextureFormat::RGBA8:      return GL_RGBA;
    case TextureFormat::RG16F:      return GL_RG;
    case TextureFormat::RGB16F:     return GL_RGB; 
    case TextureFormat::RGBA16F:    return GL_RGBA;
    case TextureFormat::R32F:       return GL_RED;
    case TextureFormat::R16F:       return GL_RED;
    case TextureFormat::RGB32F:     return GL_RGB;
    case TextureFormat::RGBA32F:    return GL_RGBA;
    case TextureFormat::DEPTH24:    return GL_DEPTH_COMPONENT;
  }
  exception("bad enum");
  return GL_RGB;
}

inline GLenum type(const TextureFormat& x) {
  switch (x) {
    case TextureFormat::RGB8:       return GL_UNSIGNED_BYTE;
    case TextureFormat::RGBA8:      return GL_UNSIGNED_BYTE;
    case TextureFormat::RG16F:      return GL_HALF_FLOAT;
    case TextureFormat::RGB16F:     return GL_HALF_FLOAT;
    case TextureFormat::RGBA16F:    return GL_HALF_FLOAT;
    case TextureFormat::R32F:       return GL_FLOAT;
    case TextureFormat::R16F:       return GL_FLOAT;
    case TextureFormat::RGB32F:     return GL_FLOAT;
    case TextureFormat::RGBA32F:    return GL_FLOAT;
    case TextureFormat::DEPTH24:    return GL_FLOAT;
  }
  exception("bad enum");
  return GL_UNSIGNED_BYTE;
}

inline GLenum native(const ShaderStageType& x) {
  switch (x) {
    case ShaderStageType::Vertex:           return GL_VERTEX_SHADER;
    case ShaderStageType::Geometry:         return GL_GEOMETRY_SHADER;
    //case ShaderStageType::Compute:          return GL_COMPUTE_SHADER;
    case ShaderStageType::Fragment:         return GL_FRAGMENT_SHADER;
  }
  exception("bad enum");
  return GL_VERTEX_SHADER;
}

inline GLenum native(const RenderBufferType& x) {
  switch (x) {
    case RenderBufferType::ColorAlpha:      return GL_RGBA;
    case RenderBufferType::Color:           return GL_RGB; 
    case RenderBufferType::Depth:           return GL_DEPTH_COMPONENT;
    case RenderBufferType::Float4:          return GL_RGBA32F;
  }
  exception("bad enum");
  return GL_RGBA;
}

inline GLenum colorAttachNum(const unsigned int i) {
  // can we just add to the 0 one? couldn't find documentation saying yes for sure.
  switch (i) {
    case 0:     return GL_COLOR_ATTACHMENT0;
    case 1:     return GL_COLOR_ATTACHMENT1;
    case 2:     return GL_COLOR_ATTACHMENT2;
    case 3:     return GL_COLOR_ATTACHMENT3;
    case 4:     return GL_COLOR_ATTACHMENT4;
    case 5:     return GL_COLOR_ATTACHMENT5;
    case 6:     return GL_COLOR_ATTACHMENT6;
    case 7:     return GL_COLOR_ATTACHMENT7;
    default:          exception("tried to use too many color attachments");
  }
  exception("bad enum");
  return GL_COLOR_ATTACHMENT0;
}

// clang-format on


// Stateful error checker
void checkGLError(bool fatal = true) {

  if (!options::enableRenderErrorChecks) {
    return;
  }

  // Map the GL error enums to strings
  GLenum err = GL_NO_ERROR;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::string errText;
    switch (err) {
    case GL_NO_ERROR:
      errText = "No error";
      break;
    case GL_INVALID_ENUM:
      errText = "Invalid enum";
      break;
    case GL_INVALID_VALUE:
      errText = "Invalid value";
      break;
    case GL_INVALID_OPERATION:
      errText = "Invalid operation";
      break;
    // case GL_STACK_OVERFLOW:    std::cerr << "Stack overflow"; break;
    // case GL_STACK_UNDERFLOW:   std::cerr << "Stack underflow"; break;
    case GL_OUT_OF_MEMORY:
      errText = "Out of memory";
      break;
    default:
      errText = "Unknown error " + std::to_string(static_cast<unsigned int>(err));
    }

    if (polyscope::options::verbosity > 0) {
      std::cout << polyscope::options::printPrefix << "Polyscope OpenGL Error!  Type: " << errText << std::endl;
    }
    if (fatal) {
      exception("OpenGl error occurred. Text: " + errText);
    }
  }
}

// Helper function to print compile logs
void printShaderInfoLog(ShaderHandle shaderHandle) {
  int logLen = 0;
  int chars = 0;
  char* log;

  glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLen);

  if (options::verbosity > 0 && logLen > 1) { // for some reason we often get logs of length 1 with no
                                              // visible characters
    log = (char*)malloc(logLen);
    glGetShaderInfoLog(shaderHandle, logLen, &chars, log);
    printf("Shader info log:\n%s\n", log);
    free(log);

    exception("shader compile failed");
  }
}
void printProgramInfoLog(GLuint handle) {
  int logLen = 0;
  int chars = 0;
  char* log;

  glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLen);

  if (options::verbosity > 0 && logLen > 1) { // for some reason we often get logs of length 1 with no
                                              // visible characters
    log = (char*)malloc(logLen);
    glGetProgramInfoLog(handle, logLen, &chars, log);
    printf("Program info log:\n%s\n", log);
    free(log);
  }
}

// =============================================================
// =================== Attribute buffer ========================
// =============================================================

GLAttributeBuffer::GLAttributeBuffer(RenderDataType dataType_, int arrayCount_)
    : AttributeBuffer(dataType_, arrayCount_) {
  glGenBuffers(1, &VBOLoc);
}

GLAttributeBuffer::~GLAttributeBuffer() {
  bind();
  glDeleteBuffers(1, &VBOLoc);
}

void GLAttributeBuffer::bind() { glBindBuffer(getTarget(), VBOLoc); }

void GLAttributeBuffer::checkType(RenderDataType targetType) {
  if (dataType != targetType) {
    throw std::invalid_argument("Tried to set GLAttributeBuffer with wrong type. Actual type: " +
                                renderDataTypeName(dataType) + "  Attempted type: " + renderDataTypeName(targetType));
  }
}

void GLAttributeBuffer::checkArray(int testArrayCount) {
  if (testArrayCount != arrayCount) {
    throw std::invalid_argument("Tried to set GLAttributeBuffer with wrong array count. Actual count: " +
                                std::to_string(arrayCount) + "  Attempted count: " + std::to_string(testArrayCount));
  }
}

GLenum GLAttributeBuffer::getTarget() { return GL_ARRAY_BUFFER; }

void GLAttributeBuffer::setData(const std::vector<glm::vec2>& data) {
  checkType(RenderDataType::Vector2Float);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::vec2) == 2 * sizeof(float), "glm::vec2 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 2 * dataSize * sizeof(float), &data[0]);

  } else {

    glBufferData(getTarget(), 2 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::vec3>& data) {
  checkType(RenderDataType::Vector3Float);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::vec3) == 3 * sizeof(float), "glm::vec3 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 3 * dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), 3 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 2>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(2);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 2 * 3 * dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), 2 * 3 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = 2 * data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 3>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(3);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 3 * 3 * dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), 3 * 3 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = 3 * data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 4>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(4);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 4 * 3 * dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), 4 * 3 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = 4 * data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::vec4>& data) {
  checkType(RenderDataType::Vector4Float);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::vec4) == 4 * sizeof(float), "glm::vec4 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, 4 * dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), 4 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<float>& data) {
  checkType(RenderDataType::Float);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, dataSize * sizeof(float), &data[0]);

  } else {
    glBufferData(getTarget(), data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<double>& data) {
  checkType(RenderDataType::Float);

  // Convert input data to floats
  std::vector<float> floatData(data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    floatData[i] = static_cast<float>(data[i]);
  }

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, dataSize * sizeof(float), &floatData[0]);

  } else {
    glBufferData(getTarget(), data.size() * sizeof(float), &floatData[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<int32_t>& data) {
  checkType(RenderDataType::Int);

  // TODO I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(int32_t) == sizeof(GLint), "int has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, dataSize * sizeof(GLint), &data[0]);

  } else {

    glBufferData(getTarget(), data.size() * sizeof(GLint), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<uint32_t>& data) {
  checkType(RenderDataType::UInt);

  // TODO I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  static_assert(sizeof(uint32_t) == sizeof(GLuint), "int has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

    glBufferSubData(getTarget(), 0, dataSize * sizeof(GLuint), &data[0]);

  } else {
    glBufferData(getTarget(), data.size() * sizeof(GLuint), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::uvec2>& data) {
  checkType(RenderDataType::Vector2UInt);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::uvec2) == 2 * sizeof(GLuint), "glm::uvec2 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
    glBufferSubData(getTarget(), 0, 2 * dataSize * sizeof(GLuint), &data[0]);
  } else {
    glBufferData(getTarget(), 2 * data.size() * sizeof(GLuint), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}
void GLAttributeBuffer::setData(const std::vector<glm::uvec3>& data) {
  checkType(RenderDataType::Vector3UInt);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::uvec3) == 3 * sizeof(GLuint), "glm::uvec3 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
    glBufferSubData(getTarget(), 0, 3 * dataSize * sizeof(GLuint), &data[0]);
  } else {
    glBufferData(getTarget(), 3 * data.size() * sizeof(GLuint), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::uvec4>& data) {
  checkType(RenderDataType::Vector4UInt);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::uvec4) == 4 * sizeof(GLuint), "glm::uvec4 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
    glBufferSubData(getTarget(), 0, 4 * dataSize * sizeof(GLuint), &data[0]);
  } else {
    glBufferData(getTarget(), 4 * data.size() * sizeof(GLuint), &data[0], GL_STATIC_DRAW);
    dataSize = data.size();
  }
}

// get single data values

float GLAttributeBuffer::getData_float(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Float) exception("bad getData type");
  bind();
  float readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(float), sizeof(float), &readValue);
  return readValue;
}
double GLAttributeBuffer::getData_double(size_t ind) { return getData_float(ind); }
glm::vec2 GLAttributeBuffer::getData_vec2(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  glm::vec2 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec2), sizeof(glm::vec2), &readValue);
  return readValue;
}
glm::vec3 GLAttributeBuffer::getData_vec3(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  glm::vec3 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec3), sizeof(glm::vec3), &readValue);
  return readValue;
}
glm::vec4 GLAttributeBuffer::getData_vec4(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  glm::vec4 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec4), sizeof(glm::vec4), &readValue);
  return readValue;
}
int GLAttributeBuffer::getData_int(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Int) exception("bad getData type");
  bind();
  GLint readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(GLint), sizeof(GLint), &readValue);
  return static_cast<int>(readValue);
}
uint32_t GLAttributeBuffer::getData_uint32(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::UInt) exception("bad getData type");
  bind();
  uint32_t readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(uint32_t), sizeof(uint32_t), &readValue);
  return readValue;
}
glm::uvec2 GLAttributeBuffer::getData_uvec2(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  glm::uvec2 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec2), sizeof(glm::uvec2), &readValue);
  return readValue;
}
glm::uvec3 GLAttributeBuffer::getData_uvec3(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  glm::uvec3 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec3), sizeof(glm::uvec3), &readValue);
  return readValue;
}
glm::uvec4 GLAttributeBuffer::getData_uvec4(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  glm::uvec4 readValue;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec4), sizeof(glm::uvec4), &readValue);
  return readValue;
}

// get ranges of data

std::vector<float> GLAttributeBuffer::getDataRange_float(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Float) exception("bad getData type");
  bind();
  std::vector<float> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(float), count * sizeof(float), &readValues.front());
  return readValues;
}

std::vector<double> GLAttributeBuffer::getDataRange_double(size_t ind, size_t count) {
  std::vector<float> floatValues = getDataRange_float(ind, count);
  std::vector<double> values(count);
  for (size_t i = 0; i < count; i++) {
    values[i] = static_cast<double>(floatValues[i]);
  }
  return values;
}

std::vector<glm::vec2> GLAttributeBuffer::getDataRange_vec2(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  std::vector<glm::vec2> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec2), count * sizeof(glm::vec2), &readValues.front());
  return readValues;
}
std::vector<glm::vec3> GLAttributeBuffer::getDataRange_vec3(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  std::vector<glm::vec3> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec3), count * sizeof(glm::vec3), &readValues.front());
  return readValues;
}
std::vector<glm::vec4> GLAttributeBuffer::getDataRange_vec4(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  std::vector<glm::vec4> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::vec4), count * sizeof(glm::vec4), &readValues.front());
  return readValues;
}
std::vector<int> GLAttributeBuffer::getDataRange_int(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Int) exception("bad getData type");
  bind();
  std::vector<GLint> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(GLint), count * sizeof(GLint), &readValues.front());

  // probably does nothing
  std::vector<int> intValues(count);
  for (size_t i = 0; i < count; i++) {
    intValues[i] = static_cast<int>(readValues[i]);
  }

  return intValues;
}
std::vector<uint32_t> GLAttributeBuffer::getDataRange_uint32(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::UInt) exception("bad getData type");
  bind();
  std::vector<uint32_t> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(uint32_t), count * sizeof(uint32_t), &readValues.front());
  return readValues;
}
std::vector<glm::uvec2> GLAttributeBuffer::getDataRange_uvec2(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec2> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec2), count * sizeof(glm::uvec2), &readValues.front());
  return readValues;
}
std::vector<glm::uvec3> GLAttributeBuffer::getDataRange_uvec3(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec3> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec3), count * sizeof(glm::uvec3), &readValues.front());
  return readValues;
}
std::vector<glm::uvec4> GLAttributeBuffer::getDataRange_uvec4(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec4> readValues;
  glGetBufferSubData(getTarget(), ind * sizeof(glm::uvec4), count * sizeof(glm::uvec4), &readValues.front());
  return readValues;
}


uint32_t GLAttributeBuffer::getNativeBufferID() { return static_cast<uint32_t>(VBOLoc); }

// =============================================================
// ==================== Texture buffer =========================
// =============================================================


// create a 1D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, const unsigned char* data)
    : TextureBuffer(1, format_, size1D) {

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_1D, handle);
  glTexImage1D(GL_TEXTURE_1D, 0, internalFormat(format), size1D, 0, formatF(format), GL_UNSIGNED_BYTE, data);
  checkGLError();

  setFilterMode(FilterMode::Nearest);
}
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, const float* data)
    : TextureBuffer(1, format_, size1D) {

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_1D, handle);
  glTexImage1D(GL_TEXTURE_1D, 0, internalFormat(format), size1D, 0, formatF(format), GL_FLOAT, data);
  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

// create a 2D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_,
                                 const unsigned char* data)
    : TextureBuffer(2, format_, sizeX_, sizeY_) {

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat(format), sizeX, sizeY, 0, formatF(format), GL_UNSIGNED_BYTE, data);
  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_, const float* data)
    : TextureBuffer(2, format_, sizeX_, sizeY_) {

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat(format), sizeX, sizeY, 0, formatF(format), GL_FLOAT, data);
  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

GLTextureBuffer::~GLTextureBuffer() { glDeleteTextures(1, &handle); }

void GLTextureBuffer::resize(unsigned int newLen) {

  TextureBuffer::resize(newLen);

  bind();
  if (dim == 1) {
    glTexImage1D(GL_TEXTURE_1D, 0, internalFormat(format), sizeX, 0, formatF(format), type(format), nullptr);
  }
  if (dim == 2) {
    exception("OpenGL error: called 1D resize on 2D texture");
  }
  checkGLError();
}

void GLTextureBuffer::resize(unsigned int newX, unsigned int newY) {

  TextureBuffer::resize(newX, newY);

  bind();
  if (dim == 1) {
    exception("OpenGL error: called 2D resize on 1D texture");
  }
  if (dim == 2) {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat(format), sizeX, sizeY, 0, formatF(format), type(format), nullptr);
  }
  checkGLError();
}

void GLTextureBuffer::setFilterMode(FilterMode newMode) {

  bind();

  switch (newMode) {
  case FilterMode::Nearest:
    glTexParameteri(textureType(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(textureType(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    break;
  case FilterMode::Linear:
    glTexParameteri(textureType(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(textureType(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    break;
  }
  glTexParameteri(textureType(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  if (dim == 2) {
    glTexParameteri(textureType(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  checkGLError();
}

void* GLTextureBuffer::getNativeHandle() { return reinterpret_cast<void*>(getHandle()); }

std::vector<float> GLTextureBuffer::getDataScalar() {
  if (dimension(format) != 1) exception("called getDataScalar on texture which does not have a 1 dimensional format");

  std::vector<float> outData;
  outData.resize(getTotalSize());

  bind();
  glGetTexImage(textureType(), 0, formatF(format), GL_FLOAT, static_cast<void*>(&outData.front()));
  checkGLError();

  return outData;
}

std::vector<glm::vec2> GLTextureBuffer::getDataVector2() {
  if (dimension(format) != 2) exception("called getDataVector2 on texture which does not have a 2 dimensional format");

  std::vector<glm::vec2> outData;
  outData.resize(getTotalSize());

  bind();
  glGetTexImage(textureType(), 0, formatF(format), GL_FLOAT, static_cast<void*>(&outData.front()));
  checkGLError();

  return outData;
}

std::vector<glm::vec3> GLTextureBuffer::getDataVector3() {
  if (dimension(format) != 3) exception("called getDataVector3 on texture which does not have a 3 dimensional format");
  exception("not implemented");

  std::vector<glm::vec3> outData;
  outData.resize(getTotalSize());

  bind();
  glGetTexImage(textureType(), 0, formatF(format), GL_FLOAT, static_cast<void*>(&outData.front()));
  checkGLError();

  return outData;
}

GLenum GLTextureBuffer::textureType() {
  if (dim == 1) {
    return GL_TEXTURE_1D;
  } else if (dim == 2) {
    return GL_TEXTURE_2D;
  }
  exception("bad texture type");
  return GL_TEXTURE_1D;
}

void GLTextureBuffer::bind() {
  glBindTexture(textureType(), handle);
  checkGLError();
}

// =============================================================
// ===================== Render buffer =========================
// =============================================================

GLRenderBuffer::GLRenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : RenderBuffer(type_, sizeX_, sizeY_) {
  glGenRenderbuffers(1, &handle);
  checkGLError();
  resize(sizeX, sizeY);
}


GLRenderBuffer::~GLRenderBuffer() { glDeleteRenderbuffers(1, &handle); }

void GLRenderBuffer::resize(unsigned int newX, unsigned int newY) {
  RenderBuffer::resize(newX, newY);
  bind();

  glRenderbufferStorage(GL_RENDERBUFFER, native(type), sizeX, sizeY);
  checkGLError();
}

void GLRenderBuffer::bind() {
  glBindRenderbuffer(GL_RENDERBUFFER, handle);
  checkGLError();
}


// =============================================================
// ===================== Framebuffer ===========================
// =============================================================

GLFrameBuffer::GLFrameBuffer(unsigned int sizeX_, unsigned int sizeY_, bool isDefault) {
  sizeX = sizeX_;
  sizeY = sizeY_;
  if (isDefault) {
    handle = 0;
  } else {
    glGenFramebuffers(1, &handle);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
  }
  checkGLError();
};

GLFrameBuffer::~GLFrameBuffer() {
  if (handle != 0) {
    glDeleteFramebuffers(1, &handle);
  }
}

void GLFrameBuffer::bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, handle);
  checkGLError();
}

void GLFrameBuffer::addColorBuffer(std::shared_ptr<RenderBuffer> renderBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLRenderBuffer> renderBuffer = std::dynamic_pointer_cast<GLRenderBuffer>(renderBufferIn);
  if (!renderBuffer) exception("tried to bind to non-GL render buffer");

  renderBuffer->bind();
  bind();

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, colorAttachNum(nColorBuffers), GL_RENDERBUFFER, renderBuffer->getHandle());
  checkGLError();
  renderBuffersColor.push_back(renderBuffer);
  nColorBuffers++;
}

void GLFrameBuffer::addDepthBuffer(std::shared_ptr<RenderBuffer> renderBufferIn) {
  // it _better_ be a GL buffer
  std::shared_ptr<GLRenderBuffer> renderBuffer = std::dynamic_pointer_cast<GLRenderBuffer>(renderBufferIn);
  if (!renderBuffer) exception("tried to bind to non-GL render buffer");

  renderBuffer->bind();
  bind();

  // Sanity checks
  // if (depthRenderBuffer != nullptr) exception("OpenGL error: already bound to render buffer");
  // if (depthTexture != nullptr) exception("OpenGL error: already bound to texture buffer");

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer->getHandle());
  checkGLError();
  renderBuffersDepth.push_back(renderBuffer);
}

void GLFrameBuffer::addColorBuffer(std::shared_ptr<TextureBuffer> textureBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLTextureBuffer> textureBuffer = std::dynamic_pointer_cast<GLTextureBuffer>(textureBufferIn);
  if (!textureBuffer) exception("tried to bind to non-GL texture buffer");

  textureBuffer->bind();
  bind();
  checkGLError();

  glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachNum(nColorBuffers), GL_TEXTURE_2D, textureBuffer->getHandle(), 0);

  checkGLError();
  textureBuffersColor.push_back(textureBuffer);
  nColorBuffers++;
}

void GLFrameBuffer::addDepthBuffer(std::shared_ptr<TextureBuffer> textureBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLTextureBuffer> textureBuffer = std::dynamic_pointer_cast<GLTextureBuffer>(textureBufferIn);
  if (!textureBuffer) exception("tried to bind to non-GL texture buffer");

  textureBuffer->bind();
  bind();
  checkGLError();

  // Sanity checks
  // if (depthRenderBuffer != nullptr) exception("OpenGL error: already bound to render buffer");
  // if (depthTexture != nullptr) exception("OpenGL error: already bound to texture buffer");

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureBuffer->getHandle(), 0);
  checkGLError();
  textureBuffersDepth.push_back(textureBuffer);
}

void GLFrameBuffer::setDrawBuffers() {
  bind();

  std::vector<GLenum> buffs;
  for (int i = 0; i < nColorBuffers; i++) {
    buffs.push_back(GL_COLOR_ATTACHMENT0 + i);
  }
  if (nColorBuffers > 0) {
    glDrawBuffers(nColorBuffers, &buffs.front());
  }
  checkGLError();
}

bool GLFrameBuffer::bindForRendering() {
  verifyBufferSizes();
  bind();

  // Check if the frame buffer is okay
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    // it would be nice to error out here, but it seems that on some platforms this happens even during normal flow.
    // For instance, on Windows we get an incomplete framebuffer when the application is minimized see
    // https://github.com/nmwsharp/polyscope/issues/36

    // exception("OpenGL error occurred: framebuffer not complete!");
    // std::cout << "OpenGL error occurred: framebuffer not complete!\n";
    return false;
  }

  render::engine->currRenderFramebuffer = this;

  // Set the viewport
  if (!viewportSet) {
    exception("OpenGL error: viewport not set for framebuffer object. Call GLFrameBuffer::setViewport()");
  }
  glViewport(viewportX, viewportY, viewportSizeX, viewportSizeY);
  render::engine->setCurrentViewport({viewportX, viewportY, viewportSizeX, viewportSizeY});
  checkGLError();

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  checkGLError();
  return true;
}

void GLFrameBuffer::clear() {
  if (!bindForRendering()) return;

  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearAlpha);
  glClearDepth(clearDepth);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

std::array<float, 4> GLFrameBuffer::readFloat4(int xPos, int yPos) {

  // if (colorRenderBuffer == nullptr || colorRenderBuffer->getType() != RenderBufferType::Float4) {
  // exception("OpenGL error: buffer is not of right type to read float4 from");
  //}

  glFlush();
  glFinish();
  bind();

  // Read from the buffer
  std::array<float, 4> result;
  glReadPixels(xPos, yPos, 1, 1, GL_RGBA, GL_FLOAT, &result);

  return result;
}

float GLFrameBuffer::readDepth(int xPos, int yPos) {

  // TODO does no error checking for the case where no depth buffer is attached

  glFlush();
  glFinish();
  bind();

  // Read from the buffer
  float result;
  glReadPixels(xPos, yPos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &result);

  return result;
}

std::vector<unsigned char> GLFrameBuffer::readBuffer() {

  glFlush();
  glFinish();

  bind();

  int w = getSizeX();
  int h = getSizeY();

  // Read from openGL
  size_t buffSize = w * h * 4;
  std::vector<unsigned char> buff(buffSize);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, &(buff.front()));

  return buff;
}

void GLFrameBuffer::blitTo(FrameBuffer* targetIn) {

  // it _better_ be a GL buffer
  GLFrameBuffer* target = dynamic_cast<GLFrameBuffer*>(targetIn);
  if (!target) exception("tried to blitTo() non-GL framebuffer");

  // target->bindForRendering();
  bindForRendering();
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->getHandle());

  glBlitFramebuffer(0, 0, getSizeX(), getSizeY(), 0, 0, target->getSizeX(), target->getSizeY(), GL_COLOR_BUFFER_BIT,
                    GL_LINEAR);
  checkGLError();
}

// =============================================================
// ==================  Shader Program  =========================
// =============================================================


GLCompiledProgram::GLCompiledProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm) : drawMode(dm) {

  // Collect attributes and uniforms from all of the shaders
  for (const ShaderStageSpecification& s : stages) {
    for (ShaderSpecUniform u : s.uniforms) {
      addUniqueUniform(u);
    }
    for (ShaderSpecAttribute a : s.attributes) {
      addUniqueAttribute(a);
    }
    for (ShaderSpecTexture t : s.textures) {
      addUniqueTexture(t);
    }
  }

  if (attributes.size() == 0) {
    throw std::invalid_argument("Uh oh... GLProgram has no attributes");
  }

  // Perform setup tasks
  compileGLProgram(stages);
  checkGLError();

  setDataLocations();
  checkGLError();
}

GLCompiledProgram::~GLCompiledProgram() { glDeleteProgram(programHandle); }

void GLCompiledProgram::compileGLProgram(const std::vector<ShaderStageSpecification>& stages) {


  // Compile all of the shaders
  std::vector<ShaderHandle> handles;
  for (const ShaderStageSpecification& s : stages) {
    ShaderHandle h = glCreateShader(native(s.stage));
    std::array<const char*, 2> srcs = {s.src.c_str(), shaderCommonSource};
    glShaderSource(h, 2, &(srcs[0]), nullptr);
    glCompileShader(h);

    // Catch the error here, so we can print shader source before re-throwing
    try {

      GLint status;
      glGetShaderiv(h, GL_COMPILE_STATUS, &status);
      if (!status) {
        printShaderInfoLog(h);
        std::cout << "Program text:" << std::endl;
        std::cout << s.src.c_str() << std::endl;
        exception("[polyscope] GL shader compile failed");
      }

      if (options::verbosity > 2) {
        printShaderInfoLog(h);
      }

      checkGLError();
    } catch (...) {
      std::cout << "GLError() after shader compilation! Program text:" << std::endl;
      std::cout << s.src.c_str() << std::endl;
      throw;
    }

    handles.push_back(h);
  }

  // Create the program and attach the shaders
  programHandle = glCreateProgram();
  for (ShaderHandle h : handles) {
    glAttachShader(programHandle, h);
  }

  // Link the program
  glLinkProgram(programHandle);
  if (options::verbosity > 2) {
    printProgramInfoLog(programHandle);
  }
  GLint status;
  glGetProgramiv(programHandle, GL_LINK_STATUS, &status);
  if (!status) {
    printProgramInfoLog(programHandle);
    exception("[polyscope] GL program compile failed");
  }

  // Delete the shaders we just compiled, they aren't used after link
  for (ShaderHandle h : handles) {
    glDeleteShader(h);
  }

  checkGLError();
}

void GLCompiledProgram::setDataLocations() {
  glUseProgram(programHandle);

  // Uniforms
  for (GLShaderUniform& u : uniforms) {
    u.location = glGetUniformLocation(programHandle, u.name.c_str());
    if (u.location == -1) {
      if (options::verbosity > 2) {
        info("failed to get location for uniform " + u.name);
      }
      // exception("failed to get location for uniform " + u.name);
    }
  }

  // Attributes
  for (GLShaderAttribute& a : attributes) {
    a.location = glGetAttribLocation(programHandle, a.name.c_str());
    if (a.location == -1) {
      info("failed to get location for attribute " + a.name);
    }
    // exception("failed to get location for attribute " + a.name);
  }

  // Textures
  for (GLShaderTexture& t : textures) {
    t.location = glGetUniformLocation(programHandle, t.name.c_str());
    if (t.location == -1) {
      info("failed to get location for texture " + t.name);
    }
    // exception("failed to get location for texture " + t.name);
  }

  checkGLError();
}

void GLCompiledProgram::addUniqueAttribute(ShaderSpecAttribute newAttribute) {
  for (GLShaderAttribute& a : attributes) {
    if (a.name == newAttribute.name) {

      // if it occurs twice, confirm that the occurences match
      if (a.type != newAttribute.type)
        exception("attribute " + a.name + " appears twice in program with different types");

      return;
    }
  }
  attributes.push_back(GLShaderAttribute{newAttribute.name, newAttribute.type, newAttribute.arrayCount, -1, nullptr});
}

void GLCompiledProgram::addUniqueUniform(ShaderSpecUniform newUniform) {
  for (GLShaderUniform& u : uniforms) {
    if (u.name == newUniform.name) {

      // if it occurs twice, confirm that the occurences match
      if (u.type != newUniform.type) exception("uniform " + u.name + " appears twice in program with different types");

      return;
    }
  }
  uniforms.push_back(GLShaderUniform{newUniform.name, newUniform.type, false, 777});
}

void GLCompiledProgram::addUniqueTexture(ShaderSpecTexture newTexture) {
  for (GLShaderTexture& t : textures) {
    if (t.name == newTexture.name) {

      // if it occurs twice, confirm that the occurences match
      if (t.dim != newTexture.dim)
        exception("texture " + t.name + " appears twice in program with different dimensions");

      return;
    }
  }
  textures.push_back(GLShaderTexture{newTexture.name, newTexture.dim, 777, false, nullptr, nullptr, 777});
}


GLShaderProgram::GLShaderProgram(const std::shared_ptr<GLCompiledProgram>& compiledProgram_)
    : ShaderProgram(compiledProgram_->getDrawMode()), uniforms(compiledProgram_->getUniforms()),
      attributes(compiledProgram_->getAttributes()), textures(compiledProgram_->getTextures()),
      compiledProgram(compiledProgram_) {

  // Create a VAO
  glGenVertexArrays(1, &vaoHandle);
  checkGLError();

  createBuffers(); // only handles texture & index things, attributes are lazily created
  checkGLError();
}

GLShaderProgram::~GLShaderProgram() {
  // TODO delete the vao and index VBO?
}

void GLShaderProgram::bindVAO() { glBindVertexArray(vaoHandle); }

void GLShaderProgram::createBuffers() {
  bindVAO();

  // Create an index buffer, if we're using one
  if (useIndex) {
    glGenBuffers(1, &indexVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  }

  // === Generate textures

  // Verify we have enough texture units
  GLint nAvailTextureUnits;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &nAvailTextureUnits);
  if ((int)textures.size() > nAvailTextureUnits) {
    throw std::invalid_argument("Attempted to load more textures than the number of available texture "
                                "units (" +
                                std::to_string(nAvailTextureUnits) + ").");
  }

  // Set indices sequentially
  uint32_t iTexture = 0;
  for (GLShaderTexture& t : textures) {
    t.index = iTexture++;
  }

  checkGLError();
}

void GLShaderProgram::setAttribute(std::string name, std::shared_ptr<AttributeBuffer> externalBuffer) {
  bindVAO();
  checkGLError();

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {

      if (a.location == -1) return; // attributes which were optimized out or something, do nothing

      // check that types match
      int compatCount = renderDataTypeCountCompatbility(a.type, externalBuffer->getType());
      if (compatCount == 0)
        throw std::invalid_argument("Tried to set attribute " + name + " to incompatibile type. Attribute " +
                                    renderDataTypeName(a.type) + " set with buffer of type " +
                                    renderDataTypeName(externalBuffer->getType()));

      // check multiple-set errors (duplicates in externalBuffers list?)
      if (a.buff) throw std::invalid_argument("attribute " + name + " is already set");

      // cast to the engine type (booooooo)
      std::shared_ptr<GLAttributeBuffer> engineExtBuff = std::dynamic_pointer_cast<GLAttributeBuffer>(externalBuffer);
      if (!engineExtBuff) throw std::invalid_argument("attribute " + name + " external buffer engine type cast failed");

      a.buff = engineExtBuff;
      checkGLError();

      a.buff->bind();
      checkGLError();

      assignBufferToVAO(a);
      checkGLError();
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::assignBufferToVAO(GLShaderAttribute& a) {
  bindVAO();
  a.buff->bind();
  checkGLError();

  // Choose the correct type for the buffer
  for (int iArrInd = 0; iArrInd < a.arrayCount; iArrInd++) {

    glEnableVertexAttribArray(a.location + iArrInd);

    switch (a.type) {
    case RenderDataType::Float:
      glVertexAttribPointer(a.location + iArrInd, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 1 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(float) * 1 * iArrInd));
      break;
    case RenderDataType::Int:
      glVertexAttribPointer(a.location + iArrInd, 1, GL_INT, GL_FALSE, sizeof(int) * 1 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(int) * 1 * iArrInd));
      break;
    case RenderDataType::UInt:
      glVertexAttribPointer(a.location + iArrInd, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t) * 1 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(uint32_t) * 1 * iArrInd));
      break;
    case RenderDataType::Vector2Float:
      glVertexAttribPointer(a.location + iArrInd, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(float) * 2 * iArrInd));
      break;
    case RenderDataType::Vector3Float:
      glVertexAttribPointer(a.location + iArrInd, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(float) * 3 * iArrInd));
      break;
    case RenderDataType::Vector4Float:
      glVertexAttribPointer(a.location + iArrInd, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(float) * 4 * iArrInd));
      break;
    case RenderDataType::Vector2UInt:
      glVertexAttribPointer(a.location + iArrInd, 2, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t) * 2 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(uint32_t) * 2 * iArrInd));
      break;
    case RenderDataType::Vector3UInt:
      glVertexAttribPointer(a.location + iArrInd, 3, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t) * 3 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(uint32_t) * 3 * iArrInd));
      break;
    case RenderDataType::Vector4UInt:
      glVertexAttribPointer(a.location + iArrInd, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t) * 4 * a.arrayCount,
                            reinterpret_cast<void*>(sizeof(uint32_t) * 4 * iArrInd));
      break;
    default:
      throw std::invalid_argument("Unrecognized GLShaderAttribute type");
      break;
    }
  }

  checkGLError();
}

void GLShaderProgram::createBuffer(GLShaderAttribute& a) {
  if (a.location == -1) return;

  // generate the buffer if needed
  std::shared_ptr<AttributeBuffer> newBuff = glEngine->generateAttributeBuffer(a.type, a.arrayCount);
  std::shared_ptr<GLAttributeBuffer> engineNewBuff = std::dynamic_pointer_cast<GLAttributeBuffer>(newBuff);
  if (!engineNewBuff) throw std::invalid_argument("buffer type cast failed");
  a.buff = engineNewBuff;

  assignBufferToVAO(a);

  checkGLError();
}

void GLShaderProgram::ensureBufferExists(GLShaderAttribute& a) {
  if (a.location != -1 && !a.buff) {
    createBuffer(a);
  }
}

bool GLShaderProgram::hasUniform(std::string name) {
  for (GLShaderUniform& u : uniforms) {
    if (u.name == name && u.location != -1) {
      return true;
    }
  }
  return false;
}

// Set an integer
void GLShaderProgram::setUniform(std::string name, int val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Int) {
        glUniform1i(u.location, val);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set an unsigned integer
void GLShaderProgram::setUniform(std::string name, unsigned int val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::UInt) {
        glUniform1ui(u.location, val);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a float
void GLShaderProgram::setUniform(std::string name, float val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Float) {
        glUniform1f(u.location, val);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a double --- WARNING casts down to float
void GLShaderProgram::setUniform(std::string name, double val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Float) {
        glUniform1f(u.location, static_cast<float>(val));
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a 4x4 uniform matrix
// TODO why do we use a pointer here... makes no sense
void GLShaderProgram::setUniform(std::string name, float* val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Matrix44Float) {
        glUniformMatrix4fv(u.location, 1, false, val);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vector2 uniform
void GLShaderProgram::setUniform(std::string name, glm::vec2 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector2Float) {
        glUniform2f(u.location, val.x, val.y);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vector3 uniform
void GLShaderProgram::setUniform(std::string name, glm::vec3 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector3Float) {
        glUniform3f(u.location, val.x, val.y, val.z);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vector4 uniform
void GLShaderProgram::setUniform(std::string name, glm::vec4 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector4Float) {
        glUniform4f(u.location, val.x, val.y, val.z, val.w);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vector3 uniform from a float array
void GLShaderProgram::setUniform(std::string name, std::array<float, 3> val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector3Float) {
        glUniform3f(u.location, val[0], val[1], val[2]);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vec4 uniform
void GLShaderProgram::setUniform(std::string name, float x, float y, float z, float w) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector4Float) {
        glUniform4f(u.location, x, y, z, w);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a uint vector2 uniform
void GLShaderProgram::setUniform(std::string name, glm::uvec2 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector2UInt) {
        glUniform2ui(u.location, val.x, val.y);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a uint vector3 uniform
void GLShaderProgram::setUniform(std::string name, glm::uvec3 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector3UInt) {
        glUniform3ui(u.location, val.x, val.y, val.z);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a uint vector4 uniform
void GLShaderProgram::setUniform(std::string name, glm::uvec4 val) {
  glUseProgram(compiledProgram->getHandle());

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.location == -1) return;
      if (u.type == RenderDataType::Vector4UInt) {
        glUniform4ui(u.location, val.x, val.y, val.z, val.w);
        u.isSet = true;
      } else {
        throw std::invalid_argument("Tried to set GLShaderUniform with wrong type");
      }
      return;
    }
  }
  throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

bool GLShaderProgram::hasAttribute(std::string name) {
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      return true;
    }
  }
  return false;
}

bool GLShaderProgram::attributeIsSet(std::string name) {
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      return a.buff->isSet();
    }
  }
  return false;
}

std::shared_ptr<AttributeBuffer> GLShaderProgram::getAttributeBuffer(std::string name) {
  // WARNING: may be null if the attribute was optimized out
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      return a.buff;
    }
  }
  throw std::invalid_argument("No attribute with name " + name);
  return nullptr;
};


void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec2>& data) {
  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec3>& data) {
  glBindVertexArray(vaoHandle); // TODO remove these?

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec4>& data) {
  glBindVertexArray(vaoHandle);

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<float>& data) {
  glBindVertexArray(vaoHandle);

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<double>& data) {
  glBindVertexArray(vaoHandle);

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<int32_t>& data) {
  glBindVertexArray(vaoHandle);

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<uint32_t>& data) {
  glBindVertexArray(vaoHandle);

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name && a.location != -1) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

bool GLShaderProgram::hasTexture(std::string name) {
  for (GLShaderTexture& t : textures) {
    if (t.name == name && t.location != -1) {
      return true;
    }
  }
  return false;
}

bool GLShaderProgram::textureIsSet(std::string name) {
  for (GLShaderTexture& t : textures) {
    if (t.name == name && t.location != -1) {
      return t.isSet;
    }
  }
  return false;
}

void GLShaderProgram::setTexture1D(std::string name, unsigned char* texData, unsigned int length) {
  throw std::invalid_argument("This code hasn't been testded yet.");

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name || t.location == -1) continue;

    if (t.isSet) {
      throw std::invalid_argument("Attempted to set texture twice");
    }

    if (t.dim != 1) {
      throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
    }

    // Create a new texture object
    t.textureBufferOwned.reset(new GLTextureBuffer(TextureFormat::RGB8, length, texData));
    t.textureBuffer = t.textureBufferOwned.get();


    // Set policies
    // glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    t.isSet = true;
    return;
  }

  throw std::invalid_argument("No texture with name " + name);
}

void GLShaderProgram::setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                                   bool withAlpha, bool useMipMap, bool repeat) {
  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name || t.location == -1) continue;

    if (t.isSet) {
      throw std::invalid_argument("Attempted to set texture twice");
    }

    if (t.dim != 2) {
      throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
    }

    if (withAlpha) {
      t.textureBufferOwned.reset(new GLTextureBuffer(TextureFormat::RGBA8, width, height, texData));
    } else {
      t.textureBufferOwned.reset(new GLTextureBuffer(TextureFormat::RGB8, width, height, texData));
    }
    t.textureBuffer = t.textureBufferOwned.get();


    // Set policies
    if (repeat) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Use mip maps
    if (useMipMap) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    t.isSet = true;
    return;
  }

  throw std::invalid_argument("No texture with name " + name);
}

void GLShaderProgram::setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer) {
  glUseProgram(compiledProgram->getHandle());

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name || t.location == -1) continue;

    if (t.dim != (int)textureBuffer->getDimension()) {
      throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
    }

    t.textureBuffer = dynamic_cast<GLTextureBuffer*>(textureBuffer);
    if (!t.textureBuffer) {
      throw std::invalid_argument("Bad texture in setTextureFromBuffer()");
    }

    t.isSet = true;
    return;
  }

  throw std::invalid_argument("No texture with name " + name);
}

void GLShaderProgram::setTextureFromColormap(std::string name, const std::string& colormapName, bool allowUpdate) {
  const ValueColorMap& colormap = render::engine->getColorMap(colormapName);

  // TODO switch to global shared buffers from colormap

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name || t.location == -1) continue;

    if (t.isSet && !allowUpdate) {
      throw std::invalid_argument("Attempted to set texture twice");
    }

    if (t.dim != 1) {
      throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
    }

    // Fill a buffer with the data
    unsigned int dataLength = colormap.values.size() * 3;
    std::vector<float> colorBuffer(dataLength);
    for (unsigned int i = 0; i < colormap.values.size(); i++) {
      colorBuffer[3 * i + 0] = static_cast<float>(colormap.values[i][0]);
      colorBuffer[3 * i + 1] = static_cast<float>(colormap.values[i][1]);
      colorBuffer[3 * i + 2] = static_cast<float>(colormap.values[i][2]);
    }

    // glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, colormap.values.size(), 0, GL_RGB, GL_FLOAT, &(colorBuffer[0]));
    t.textureBufferOwned = std::dynamic_pointer_cast<GLTextureBuffer>(
        engine->generateTextureBuffer(TextureFormat::RGB32F, colormap.values.size(), &(colorBuffer[0])));
    t.textureBufferOwned->setFilterMode(FilterMode::Linear);
    t.textureBuffer = t.textureBufferOwned.get();


    t.isSet = true;
    return;
  }

  throw std::invalid_argument("No texture with name " + name);
}

// TODO get rid of this and make a shared attribue buffer
void GLShaderProgram::setIndex(std::vector<std::array<unsigned int, 3>>& indices) {
  if (!useIndex) {
    throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed "
                                "drawing");
  }

  // Reshape the vector
  // Right now, the data is probably laid out in this form already... but let's
  // not be overly clever and just reshape it.
  unsigned int* rawData = new unsigned int[3 * indices.size()];
  for (unsigned int i = 0; i < indices.size(); i++) {
    rawData[3 * i + 0] = static_cast<unsigned int>(indices[i][0]);
    rawData[3 * i + 1] = static_cast<unsigned int>(indices[i][1]);
    rawData[3 * i + 2] = static_cast<unsigned int>(indices[i][2]);
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * indices.size() * sizeof(unsigned int), rawData, GL_STATIC_DRAW);

  delete[] rawData;
  indexSize = 3 * indices.size();
}

void GLShaderProgram::setIndex(std::vector<glm::uvec3>& indices) {
  if (!useIndex) {
    throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed "
                                "drawing");
  }

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::uvec3) == 3 * sizeof(GLuint), "glm::uvec3 has unexpected size/layout on this platform");

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

  indexSize = 3 * indices.size();
}

void GLShaderProgram::setIndex(std::vector<unsigned int>& indices) {
  // (This version is typically used for indexed lines)

  if (!useIndex) {
    throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed "
                                "drawing");
  }

  // Catch some cases where we forget to specify the restart index.
  // It would be nice to do a more complete check involving the data buffer, but this is simple
  // and catches most mistakes.
  if (usePrimitiveRestart && !primitiveRestartIndexSet) {
    GLuint bigThresh = static_cast<GLuint>(-1) / 2;
    for (unsigned int x : indices) {
      if (x > bigThresh) {
        throw std::invalid_argument("An unusual index was passed, but setPrimitiveRestartIndex() has not been called.");
      }
    }
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
  indexSize = indices.size();
}

// Check that uniforms and attributes are all set and of consistent size
void GLShaderProgram::validateData() {
  // Check uniforms
  for (GLShaderUniform& u : uniforms) {
    if (u.location == -1) continue;
    if (!u.isSet) {
      throw std::invalid_argument("Uniform " + u.name + " has not been set");
    }
  }

  // Check attributes
  int64_t attributeSize = -1;
  for (GLShaderAttribute a : attributes) {
    if (a.location == -1) continue;
    if (!a.buff) {
      throw std::invalid_argument("Attribute " + a.name + " has no buffer attached");
    }
    if (a.buff->getDataSize() < 0) {
      throw std::invalid_argument("Attribute " + a.name + " has not been set");
    }

    int compatCount = renderDataTypeCountCompatbility(a.type, a.buff->getType());

    if (attributeSize == -1) { // first one we've seen
      attributeSize = a.buff->getDataSize() / (a.arrayCount * compatCount);
    } else { // not the first one we've seen
      if (a.buff->getDataSize() / (a.arrayCount * compatCount) != attributeSize) {
        throw std::invalid_argument("Attributes have inconsistent size. One attribute has size " +
                                    std::to_string(attributeSize) + " and " + a.name + " has size " +
                                    std::to_string(a.buff->getDataSize()));
      }
    }
  }
  drawDataLength = static_cast<unsigned int>(attributeSize);

  // Check textures
  for (GLShaderTexture& t : textures) {
    if (t.location == -1) continue;
    if (!t.isSet) {
      throw std::invalid_argument("Texture " + t.name + " has not been set");
    }
  }

  // Check index (if applicable)
  if (useIndex) {
    if (indexSize == -1) {
      throw std::invalid_argument("Index buffer has not been filled");
    }
    drawDataLength = static_cast<unsigned int>(indexSize);
  }
}

void GLShaderProgram::setPrimitiveRestartIndex(unsigned int restartIndex_) {
  if (!usePrimitiveRestart) {
    exception("setPrimitiveRestartIndex() called, but draw mode does not support restart indices.");
  }
  restartIndex = restartIndex_;
  primitiveRestartIndexSet = true;
}

void GLShaderProgram::activateTextures() {
  for (GLShaderTexture& t : textures) {
    if (t.location == -1) continue;

    glActiveTexture(GL_TEXTURE0 + t.index);
    t.textureBuffer->bind();
    glUniform1i(t.location, t.index);
  }
}

void GLShaderProgram::draw() {
  validateData();

  glUseProgram(compiledProgram->getHandle());
  glBindVertexArray(vaoHandle);

  if (usePrimitiveRestart) {
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(restartIndex);
  }

  activateTextures();

  switch (drawMode) {
  case DrawMode::Points:
    glDrawArrays(GL_POINTS, 0, drawDataLength);
    break;
  case DrawMode::Triangles:
    glDrawArrays(GL_TRIANGLES, 0, drawDataLength);
    break;
  case DrawMode::Lines:
    glDrawArrays(GL_LINES, 0, drawDataLength);
    break;
  case DrawMode::TrianglesAdjacency:
    glDrawArrays(GL_TRIANGLES_ADJACENCY, 0, drawDataLength);
    break;
  case DrawMode::LinesAdjacency:
    glDrawArrays(GL_LINES_ADJACENCY, 0, drawDataLength);
    break;
  case DrawMode::IndexedLines:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glDrawElements(GL_LINES, drawDataLength, GL_UNSIGNED_INT, 0);
    break;
  case DrawMode::IndexedLineStrip:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glDrawElements(GL_LINE_STRIP, drawDataLength, GL_UNSIGNED_INT, 0);
    break;
  case DrawMode::IndexedLinesAdjacency:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glDrawElements(GL_LINES_ADJACENCY, drawDataLength, GL_UNSIGNED_INT, 0);
    break;
  case DrawMode::IndexedLineStripAdjacency:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glDrawElements(GL_LINE_STRIP_ADJACENCY, drawDataLength, GL_UNSIGNED_INT, 0);
    break;
  case DrawMode::IndexedTriangles:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glDrawElements(GL_TRIANGLES, drawDataLength, GL_UNSIGNED_INT, 0);
    break;
  }

  if (usePrimitiveRestart) {
    glDisable(GL_PRIMITIVE_RESTART);
  }

  checkGLError();
}

GLEngine::GLEngine() {}

void GLEngine::initialize() {
  // Small callback function for GLFW errors
  auto error_print_callback = [](int error, const char* description) {
    if (polyscope::options::verbosity > 0) {
      std::cout << "GLFW emitted error: " << description << std::endl;
    }
  };

  // === Initialize glfw
  glfwSetErrorCallback(error_print_callback);
  if (!glfwInit()) {
    exception(options::printPrefix + "ERROR: Failed to initialize glfw");
  }

  // OpenGL version things
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Create the window with context
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
  mainWindow = glfwCreateWindow(view::windowWidth, view::windowHeight, options::programName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(mainWindow);
  glfwSwapInterval(1); // Enable vsync
  glfwSetWindowPos(mainWindow, view::initWindowPosX, view::initWindowPosY);

  // Set initial window size
  int newBufferWidth, newBufferHeight, newWindowWidth, newWindowHeight;
  glfwGetFramebufferSize(mainWindow, &newBufferWidth, &newBufferHeight);
  glfwGetWindowSize(mainWindow, &newWindowWidth, &newWindowHeight);
  view::bufferWidth = newBufferWidth;
  view::bufferHeight = newBufferHeight;
  view::windowWidth = newWindowWidth;
  view::windowHeight = newWindowHeight;

// === Initialize openGL
// Load openGL functions (using GLAD)
#ifndef __APPLE__
  if (!gladLoadGL()) {
    exception(options::printPrefix + "ERROR: Failed to load openGL using GLAD");
  }
#endif
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "Backend: openGL3_glfw -- "
              << "Loaded openGL version: " << glGetString(GL_VERSION) << std::endl;
  }

#ifdef __APPLE__
  // Hack to classify the process as interactive
  glfwPollEvents();
#endif

  { // Manually create the screen frame buffer
    GLFrameBuffer* glScreenBuffer = new GLFrameBuffer(view::bufferWidth, view::bufferHeight, true);
    displayBuffer.reset(glScreenBuffer);
    glScreenBuffer->bind();
    glClearColor(1., 1., 1., 0.);
    // glClearColor(0., 0., 0., 0.);
    // glClearDepth(1.);
  }

  populateDefaultShadersAndRules();
}


void GLEngine::initializeImGui() {
  bindDisplay();

  ImGui::CreateContext(); // must call once at start

  // Set up ImGUI glfw bindings
  ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
  const char* glsl_version = "#version 150";
  ImGui_ImplOpenGL3_Init(glsl_version);

  configureImGui();
}

void GLEngine::shutdownImGui() {
  // ImGui shutdown things
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void GLEngine::swapDisplayBuffers() {
  bindDisplay();
  glfwSwapBuffers(mainWindow);
}

std::vector<unsigned char> GLEngine::readDisplayBuffer() {
  // TODO do we need to bind here?

  glFlush();
  glFinish();

  // Get buffer size
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int w = viewport[2];
  int h = viewport[3];

  // Read from openGL
  size_t buffSize = w * h * 4;
  std::vector<unsigned char> buff(buffSize);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, &(buff.front()));

  return buff;
}


void GLEngine::checkError(bool fatal) { checkGLError(fatal); }

void GLEngine::makeContextCurrent() { glfwMakeContextCurrent(mainWindow); }

void GLEngine::focusWindow() { glfwFocusWindow(mainWindow); }

void GLEngine::showWindow() { glfwShowWindow(mainWindow); }

void GLEngine::hideWindow() {
  glfwHideWindow(mainWindow);
  glfwPollEvents(); // this shouldn't be necessary, but seems to be needed at least on macOS. Perhaps realted to a
                    // glfw bug? e.g. https://github.com/glfw/glfw/issues/1300 and related bugs
}

void GLEngine::updateWindowSize(bool force) {
  int newBufferWidth, newBufferHeight, newWindowWidth, newWindowHeight;
  glfwGetFramebufferSize(mainWindow, &newBufferWidth, &newBufferHeight);
  glfwGetWindowSize(mainWindow, &newWindowWidth, &newWindowHeight);
  if (force || newBufferWidth != view::bufferWidth || newBufferHeight != view::bufferHeight ||
      newWindowHeight != view::windowHeight || newWindowWidth != view::windowWidth) {
    // Basically a resize callback
    requestRedraw();

    // prevent any division by zero for e.g. aspect ratio calcs
    if (newBufferHeight == 0) newBufferHeight = 1;
    if (newWindowHeight == 0) newWindowHeight = 1;

    view::bufferWidth = newBufferWidth;
    view::bufferHeight = newBufferHeight;
    view::windowWidth = newWindowWidth;
    view::windowHeight = newWindowHeight;

    render::engine->resizeScreenBuffers();
    render::engine->setScreenBufferViewports();
  }
}


void GLEngine::applyWindowSize() {
  glfwSetWindowSize(mainWindow, view::windowWidth, view::windowHeight);
  updateWindowSize(true);
}


void GLEngine::setWindowResizable(bool newVal) {
  glfwSetWindowAttrib(mainWindow, GLFW_RESIZABLE, newVal ? GLFW_TRUE : GLFW_FALSE);
}

bool GLEngine::getWindowResizable() { return glfwGetWindowAttrib(mainWindow, GLFW_RESIZABLE); }

std::tuple<int, int> GLEngine::getWindowPos() {
  int x, y;
  glfwGetWindowPos(mainWindow, &x, &y);
  return std::tuple<int, int>{x, y};
}

bool GLEngine::windowRequestsClose() {
  bool shouldClose = glfwWindowShouldClose(mainWindow);
  if (shouldClose) {
    glfwSetWindowShouldClose(mainWindow, false); // un-set the state bit so we can close again
    return true;
  }
  return false;
}

void GLEngine::pollEvents() { glfwPollEvents(); }

bool GLEngine::isKeyPressed(char c) {
  if (c >= '0' && c <= '9') return ImGui::IsKeyPressed(GLFW_KEY_0 + (c - '0'));
  if (c >= 'a' && c <= 'z') return ImGui::IsKeyPressed(GLFW_KEY_A + (c - 'a'));
  exception("keyPressed only supports 0-9, a-z");
  return false;
}

void GLEngine::ImGuiNewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();
}

void GLEngine::ImGuiRender() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLEngine::setDepthMode(DepthMode newMode) {
  switch (newMode) {
  case DepthMode::Less:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    break;
  case DepthMode::LEqual:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    break;
  case DepthMode::LEqualReadOnly:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    break;
  case DepthMode::PassReadOnly:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    break;
  case DepthMode::Greater:
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    break;
  case DepthMode::Disable:
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // doesn't actually matter
    break;
  }
}

void GLEngine::setBlendMode(BlendMode newMode) {
  switch (newMode) {
  case BlendMode::Over:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case BlendMode::AlphaOver:
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    break;
  case BlendMode::OverNoWrite:
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    break;
  case BlendMode::Under:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    break;
  case BlendMode::Zero:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ZERO);
    break;
  case BlendMode::WeightedAdd:
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
    break;
  case BlendMode::Source:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
    break;
  case BlendMode::Disable:
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // doesn't actually matter
    break;
  }
}

void GLEngine::setColorMask(std::array<bool, 4> mask) { glColorMask(mask[0], mask[1], mask[2], mask[3]); }

void GLEngine::setBackfaceCull(bool newVal) {
  if (newVal) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
  } else {
    glDisable(GL_CULL_FACE);
  }
}

std::string GLEngine::getClipboardText() {
  std::string clipboardData = ImGui::GetClipboardText();
  return clipboardData;
}

void GLEngine::setClipboardText(std::string text) { ImGui::SetClipboardText(text.c_str()); }

void GLEngine::applyTransparencySettings() {
  // Remove any old transparency-related rules
  switch (transparencyMode) {
  case TransparencyMode::None: {
    setBlendMode();
    setDepthMode();
    break;
  }
  case TransparencyMode::Simple: {
    setBlendMode(BlendMode::WeightedAdd);
    setDepthMode(DepthMode::Disable);
    break;
  }
  case TransparencyMode::Pretty: {
    setBlendMode(BlendMode::Disable);
    setDepthMode();
    break;
  }
  }
}

void GLEngine::setFrontFaceCCW(bool newVal) {
  if (newVal == frontFaceCCW) return;
  frontFaceCCW = newVal;
  if (frontFaceCCW) {
    glFrontFace(GL_CCW);
  } else {
    glFrontFace(GL_CW);
  }
}

// == Factories


std::shared_ptr<AttributeBuffer> GLEngine::generateAttributeBuffer(RenderDataType dataType_, int arrayCount_) {
  GLAttributeBuffer* newA = new GLAttributeBuffer(dataType_, arrayCount_);
  return std::shared_ptr<AttributeBuffer>(newA);
}

std::shared_ptr<TextureBuffer> GLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               const unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}

std::shared_ptr<TextureBuffer> GLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               const float* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> GLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_, const unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, sizeX_, sizeY_, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> GLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_, const float* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, sizeX_, sizeY_, data);
  return std::shared_ptr<TextureBuffer>(newT);
}

std::shared_ptr<RenderBuffer> GLEngine::generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                             unsigned int sizeY_) {
  GLRenderBuffer* newR = new GLRenderBuffer(type, sizeX_, sizeY_);
  return std::shared_ptr<RenderBuffer>(newR);
}

std::shared_ptr<FrameBuffer> GLEngine::generateFrameBuffer(unsigned int sizeX_, unsigned int sizeY_) {
  GLFrameBuffer* newF = new GLFrameBuffer(sizeX_, sizeY_);
  return std::shared_ptr<FrameBuffer>(newF);
}

std::string GLEngine::programKeyFromRules(const std::string& programName, const std::vector<std::string>& rules,
                                          ShaderReplacementDefaults defaults) {

  std::stringstream builder;

  // program name comes first
  builder << "$PROGRAMNAME: ";
  builder << programName << "#";

  // then rules
  builder << "  $RULES: ";
  for (const std::string& s : rules) builder << s << "# ";

  // then rules from the defaults
  builder << "  $DEFAULTS: ";
  switch (defaults) {
  case ShaderReplacementDefaults::SceneObject: {
    for (const std::string& s : defaultRules_sceneObject) builder << s << "# ";
    break;
  }
  case ShaderReplacementDefaults::Pick: {
    for (const std::string& s : defaultRules_pick) builder << s << "# ";
    break;
  }
  case ShaderReplacementDefaults::Process: {
    for (const std::string& s : defaultRules_process) builder << s << "# ";
    break;
  }
  case ShaderReplacementDefaults::None: {
    break;
  }
  }

  return builder.str();
}

std::shared_ptr<GLCompiledProgram> GLEngine::getCompiledProgram(const std::string& programName,
                                                                const std::vector<std::string>& customRules,
                                                                ShaderReplacementDefaults defaults) {

  // Build a cache key for the program
  std::string progKey = programKeyFromRules(programName, customRules, defaults);


  // If the cache doesn't already contain the program, create it and add to cache
  if (compiledProgamCache.find(progKey) == compiledProgamCache.end()) {

    if (polyscope::options::verbosity > 3) polyscope::info("compiling shader program " + progKey);

    // == Compile the program

    // Get the list of shaders comprising the program from the global cache
    if (registeredShaderPrograms.find(programName) == registeredShaderPrograms.end()) {
      exception("No shader program with name [" + programName + "] registered.");
    }
    const std::vector<ShaderStageSpecification>& stages = registeredShaderPrograms[programName].first;
    DrawMode dm = registeredShaderPrograms[programName].second;

    // Add in the default rules
    std::vector<std::string> fullCustomRules = customRules;
    switch (defaults) {
    case ShaderReplacementDefaults::SceneObject: {
      fullCustomRules.insert(fullCustomRules.begin(), defaultRules_sceneObject.begin(), defaultRules_sceneObject.end());
      break;
    }
    case ShaderReplacementDefaults::Pick: {
      fullCustomRules.insert(fullCustomRules.begin(), defaultRules_pick.begin(), defaultRules_pick.end());
      break;
    }
    case ShaderReplacementDefaults::Process: {
      fullCustomRules.insert(fullCustomRules.begin(), defaultRules_process.begin(), defaultRules_process.end());
      break;
    }
    case ShaderReplacementDefaults::None: {
      break;
    }
    }

    // Prepare rule substitutions
    std::vector<ShaderReplacementRule> rules;
    for (auto it = fullCustomRules.begin(); it < fullCustomRules.end(); it++) {
      std::string& ruleName = *it;

      // Only process each rule the first time it is seen
      if (std::find(fullCustomRules.begin(), it, ruleName) != it) {
        continue;
      }

      if (registeredShaderRules.find(ruleName) == registeredShaderRules.end()) {
        exception("No shader replacement rule with name [" + ruleName + "] registered.");
      }
      ShaderReplacementRule& thisRule = registeredShaderRules[ruleName];
      rules.push_back(thisRule);
    }

    // Actually apply rule substitutions
    std::vector<ShaderStageSpecification> updatedStages = applyShaderReplacements(stages, rules);

    // Create a new compiled program (GL work happens in the constructor)
    compiledProgamCache[progKey] = std::shared_ptr<GLCompiledProgram>(new GLCompiledProgram(updatedStages, dm));
  }

  // Now that the cache must contain the compiled program, just return it
  return compiledProgamCache[progKey];
}

std::shared_ptr<ShaderProgram> GLEngine::requestShader(const std::string& programName,
                                                       const std::vector<std::string>& customRules,
                                                       ShaderReplacementDefaults defaults) {
  GLShaderProgram* newP = new GLShaderProgram(getCompiledProgram(programName, customRules, defaults));
  return std::shared_ptr<ShaderProgram>(newP);
}


void GLEngine::registerShaderProgram(const std::string& name, const std::vector<ShaderStageSpecification>& spec,
                                     const DrawMode& dm) {
  registeredShaderPrograms.insert({name, {spec, dm}});
}

void GLEngine::registerShaderRule(const std::string& name, const ShaderReplacementRule& rule) {
  registeredShaderRules.insert({name, rule});
}

void GLEngine::populateDefaultShadersAndRules() {
  // clang-format off

  // == Load general base shaders
  registerShaderProgram("MESH", {FLEX_MESH_VERT_SHADER, FLEX_MESH_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("SLICE_TETS", {SLICE_TETS_VERT_SHADER, SLICE_TETS_GEOM_SHADER, SLICE_TETS_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("INDEXED_MESH", {FLEX_MESH_VERT_SHADER, FLEX_MESH_FRAG_SHADER}, DrawMode::IndexedTriangles);
  registerShaderProgram("RAYCAST_SPHERE", {FLEX_SPHERE_VERT_SHADER, FLEX_SPHERE_GEOM_SHADER, FLEX_SPHERE_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("POINT_QUAD", {FLEX_POINTQUAD_VERT_SHADER, FLEX_POINTQUAD_GEOM_SHADER, FLEX_POINTQUAD_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("RAYCAST_VECTOR", {FLEX_VECTOR_VERT_SHADER, FLEX_VECTOR_GEOM_SHADER, FLEX_VECTOR_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("RAYCAST_TANGENT_VECTOR", {FLEX_TANGENT_VECTOR_VERT_SHADER, FLEX_VECTOR_GEOM_SHADER, FLEX_VECTOR_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("RAYCAST_CYLINDER", {FLEX_CYLINDER_VERT_SHADER, FLEX_CYLINDER_GEOM_SHADER, FLEX_CYLINDER_FRAG_SHADER}, DrawMode::Points);
  registerShaderProgram("HISTOGRAM", {HISTOGRAM_VERT_SHADER, HISTOGRAM_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("GROUND_PLANE_TILE", {GROUND_PLANE_VERT_SHADER, GROUND_PLANE_TILE_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("GROUND_PLANE_TILE_REFLECT", {GROUND_PLANE_VERT_SHADER, GROUND_PLANE_TILE_REFLECT_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("GROUND_PLANE_SHADOW", {GROUND_PLANE_VERT_SHADER, GROUND_PLANE_SHADOW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("MAP_LIGHT", {TEXTURE_DRAW_VERT_SHADER, MAP_LIGHT_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("RIBBON", {RIBBON_VERT_SHADER, RIBBON_GEOM_SHADER, RIBBON_FRAG_SHADER}, DrawMode::IndexedLineStripAdjacency);
  registerShaderProgram("SLICE_PLANE", {SLICE_PLANE_VERT_SHADER, SLICE_PLANE_FRAG_SHADER}, DrawMode::Triangles);

  registerShaderProgram("TEXTURE_DRAW_PLAIN", {TEXTURE_DRAW_VERT_SHADER, PLAIN_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("TEXTURE_DRAW_DOT3", {TEXTURE_DRAW_VERT_SHADER, DOT3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("TEXTURE_DRAW_MAP3", {TEXTURE_DRAW_VERT_SHADER, MAP3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("TEXTURE_DRAW_SPHEREBG", {SPHEREBG_DRAW_VERT_SHADER, SPHEREBG_DRAW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("TEXTURE_DRAW_RENDERIMAGE_PLAIN", {TEXTURE_DRAW_VERT_SHADER, PLAIN_RENDERIMAGE_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
  registerShaderProgram("COMPOSITE_PEEL", {TEXTURE_DRAW_VERT_SHADER, COMPOSITE_PEEL}, DrawMode::Triangles);
  registerShaderProgram("DEPTH_COPY", {TEXTURE_DRAW_VERT_SHADER, DEPTH_COPY}, DrawMode::Triangles);
  registerShaderProgram("DEPTH_TO_MASK", {TEXTURE_DRAW_VERT_SHADER, DEPTH_TO_MASK}, DrawMode::Triangles);
  registerShaderProgram("SCALAR_TEXTURE_COLORMAP", {TEXTURE_DRAW_VERT_SHADER, SCALAR_TEXTURE_COLORMAP}, DrawMode::Triangles);
  registerShaderProgram("BLUR_RGB", {TEXTURE_DRAW_VERT_SHADER, BLUR_RGB}, DrawMode::Triangles);
  registerShaderProgram("TRANSFORMATION_GIZMO_ROT", {TRANSFORMATION_GIZMO_ROT_VERT, TRANSFORMATION_GIZMO_ROT_FRAG}, DrawMode::Triangles);

  // === Load rules

  // Utility rules
  registerShaderRule("GLSL_VERSION", GLSL_VERSION);
  registerShaderRule("GLOBAL_FRAGMENT_FILTER", GLOBAL_FRAGMENT_FILTER);
  registerShaderRule("DOWNSAMPLE_RESOLVE_1", DOWNSAMPLE_RESOLVE_1);
  registerShaderRule("DOWNSAMPLE_RESOLVE_2", DOWNSAMPLE_RESOLVE_2);
  registerShaderRule("DOWNSAMPLE_RESOLVE_3", DOWNSAMPLE_RESOLVE_3);
  registerShaderRule("DOWNSAMPLE_RESOLVE_4", DOWNSAMPLE_RESOLVE_4);
  
  registerShaderRule("TRANSPARENCY_STRUCTURE", TRANSPARENCY_STRUCTURE);
  registerShaderRule("TRANSPARENCY_RESOLVE_SIMPLE", TRANSPARENCY_RESOLVE_SIMPLE);
  registerShaderRule("TRANSPARENCY_PEEL_STRUCTURE", TRANSPARENCY_PEEL_STRUCTURE);
  registerShaderRule("TRANSPARENCY_PEEL_GROUND", TRANSPARENCY_PEEL_GROUND);
  
  registerShaderRule("GENERATE_VIEW_POS", GENERATE_VIEW_POS);
  registerShaderRule("CULL_POS_FROM_VIEW", CULL_POS_FROM_VIEW);

  // Lighting and shading things
  registerShaderRule("LIGHT_MATCAP", LIGHT_MATCAP);
  registerShaderRule("LIGHT_PASSTHRU", LIGHT_PASSTHRU);
  registerShaderRule("SHADE_BASECOLOR", SHADE_BASECOLOR);
  registerShaderRule("SHADE_COLOR", SHADE_COLOR);
  registerShaderRule("SHADE_COLORMAP_VALUE", SHADE_COLORMAP_VALUE);
  registerShaderRule("SHADE_COLORMAP_ANGULAR2", SHADE_COLORMAP_ANGULAR2);
  registerShaderRule("SHADE_GRID_VALUE2", SHADE_GRID_VALUE2);
  registerShaderRule("SHADE_CHECKER_VALUE2", SHADE_CHECKER_VALUE2);
  registerShaderRule("SHADEVALUE_MAG_VALUE2", SHADEVALUE_MAG_VALUE2);
  registerShaderRule("ISOLINE_STRIPE_VALUECOLOR", ISOLINE_STRIPE_VALUECOLOR);
  registerShaderRule("CHECKER_VALUE2COLOR", CHECKER_VALUE2COLOR);
 
  // Texture and image things
  registerShaderRule("TEXTURE_ORIGIN_UPPERLEFT", TEXTURE_ORIGIN_UPPERLEFT);
  registerShaderRule("TEXTURE_ORIGIN_LOWERLEFT", TEXTURE_ORIGIN_LOWERLEFT);
  registerShaderRule("TEXTURE_SET_TRANSPARENCY", TEXTURE_SET_TRANSPARENCY);
  registerShaderRule("TEXTURE_SHADE_COLOR", TEXTURE_SHADE_COLOR);
  registerShaderRule("TEXTURE_PROPAGATE_VALUE", TEXTURE_PROPAGATE_VALUE);
  registerShaderRule("TEXTURE_BILLBOARD_FROM_UNIFORMS", TEXTURE_BILLBOARD_FROM_UNIFORMS);

  // mesh things
  registerShaderRule("MESH_WIREFRAME", MESH_WIREFRAME);
  registerShaderRule("MESH_WIREFRAME_ONLY", MESH_WIREFRAME_ONLY);
  registerShaderRule("MESH_COMPUTE_NORMAL_FROM_POSITION", MESH_COMPUTE_NORMAL_FROM_POSITION);
  registerShaderRule("MESH_BACKFACE_NORMAL_FLIP", MESH_BACKFACE_NORMAL_FLIP);
  registerShaderRule("MESH_BACKFACE_DIFFERENT", MESH_BACKFACE_DIFFERENT);
  registerShaderRule("MESH_BACKFACE_DARKEN", MESH_BACKFACE_DARKEN);
  registerShaderRule("MESH_PROPAGATE_VALUE", MESH_PROPAGATE_VALUE);
  registerShaderRule("MESH_PROPAGATE_VALUE2", MESH_PROPAGATE_VALUE2);
  registerShaderRule("MESH_PROPAGATE_COLOR", MESH_PROPAGATE_COLOR);
  registerShaderRule("MESH_PROPAGATE_HALFEDGE_VALUE", MESH_PROPAGATE_HALFEDGE_VALUE);
  registerShaderRule("MESH_PROPAGATE_CULLPOS", MESH_PROPAGATE_CULLPOS);
  registerShaderRule("MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE", MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE);
  registerShaderRule("MESH_PROPAGATE_PICK", MESH_PROPAGATE_PICK);
  registerShaderRule("MESH_PROPAGATE_PICK_SIMPLE", MESH_PROPAGATE_PICK_SIMPLE);

  // sphere things
  registerShaderRule("SPHERE_PROPAGATE_VALUE", SPHERE_PROPAGATE_VALUE);
  registerShaderRule("SPHERE_PROPAGATE_VALUE2", SPHERE_PROPAGATE_VALUE2);
  registerShaderRule("SPHERE_PROPAGATE_COLOR", SPHERE_PROPAGATE_COLOR);
  registerShaderRule("SPHERE_CULLPOS_FROM_CENTER", SPHERE_CULLPOS_FROM_CENTER);
  registerShaderRule("SPHERE_CULLPOS_FROM_CENTER_QUAD", SPHERE_CULLPOS_FROM_CENTER_QUAD);
  registerShaderRule("SPHERE_VARIABLE_SIZE", SPHERE_VARIABLE_SIZE);

  // vector things
  registerShaderRule("VECTOR_PROPAGATE_COLOR", VECTOR_PROPAGATE_COLOR);
  registerShaderRule("VECTOR_CULLPOS_FROM_TAIL", VECTOR_CULLPOS_FROM_TAIL);
  registerShaderRule("TRANSFORMATION_GIZMO_VEC", TRANSFORMATION_GIZMO_VEC);

  // cylinder things
  registerShaderRule("CYLINDER_PROPAGATE_VALUE", CYLINDER_PROPAGATE_VALUE);
  registerShaderRule("CYLINDER_PROPAGATE_BLEND_VALUE", CYLINDER_PROPAGATE_BLEND_VALUE);
  registerShaderRule("CYLINDER_PROPAGATE_COLOR", CYLINDER_PROPAGATE_COLOR);
  registerShaderRule("CYLINDER_PROPAGATE_BLEND_COLOR", CYLINDER_PROPAGATE_BLEND_COLOR);
  registerShaderRule("CYLINDER_PROPAGATE_PICK", CYLINDER_PROPAGATE_PICK);
  registerShaderRule("CYLINDER_CULLPOS_FROM_MID", CYLINDER_CULLPOS_FROM_MID);
  registerShaderRule("CYLINDER_VARIABLE_SIZE", CYLINDER_VARIABLE_SIZE);

  // marching tets things
  registerShaderRule("SLICE_TETS_BASECOLOR_SHADE", SLICE_TETS_BASECOLOR_SHADE);
  registerShaderRule("SLICE_TETS_PROPAGATE_VALUE", SLICE_TETS_PROPAGATE_VALUE);
  registerShaderRule("SLICE_TETS_PROPAGATE_VECTOR", SLICE_TETS_PROPAGATE_VECTOR);
  registerShaderRule("SLICE_TETS_VECTOR_COLOR", SLICE_TETS_VECTOR_COLOR);
  registerShaderRule("SLICE_TETS_MESH_WIREFRAME", SLICE_TETS_MESH_WIREFRAME);

  // clang-format on
};

void GLEngine::createSlicePlaneFliterRule(std::string uniquePostfix) {
  registeredShaderRules.insert({"SLICE_PLANE_CULL_" + uniquePostfix, generateSlicePlaneRule(uniquePostfix)});
}


} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

#else

#include <stdexcept>

#include "polyscope/messages.h"

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {
void initializeRenderEngine() { exception("Polyscope was not compiled with support for backend: openGL3_glfw"); }
} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

#endif

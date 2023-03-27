// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#ifdef POLYSCOPE_BACKEND_OPENGL_MOCK_ENABLED
#include "polyscope/render/mock_opengl/mock_gl_engine.h"

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

namespace polyscope {
namespace render {
namespace backend_openGL_mock {


MockGLEngine* glEngine = nullptr; // alias for engine pointer

void initializeRenderEngine() {
  glEngine = new MockGLEngine();
  engine = glEngine;
  glEngine->initialize();
  engine->allocateGlobalBuffersAndPrograms();
}

// == Map enums to native values

void checkGLError(bool fatal = true) {}

// =============================================================
// =================== Attribute buffer ========================
// =============================================================

GLAttributeBuffer::GLAttributeBuffer(RenderDataType dataType_, int arrayCount_)
    : AttributeBuffer(dataType_, arrayCount_) {}

GLAttributeBuffer::~GLAttributeBuffer() { bind(); }

void GLAttributeBuffer::bind() {}

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

void GLAttributeBuffer::setData(const std::vector<glm::vec2>& data) {
  checkType(RenderDataType::Vector2Float);

  // sanity check that the data array has the expected layout for the memcopy below
  static_assert(sizeof(glm::vec2) == 2 * sizeof(float), "glm::vec2 has unexpected size/layout on this platform");

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {

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

  } else {
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 2>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(2);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {
    dataSize = 2 * data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 3>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(3);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {
    dataSize = 3 * data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<std::array<glm::vec3, 4>>& data) {
  checkType(RenderDataType::Vector3Float);
  checkArray(4);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {
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

  } else {
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<float>& data) {
  checkType(RenderDataType::Float);

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {
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

  } else {
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<int32_t>& data) {
  checkType(RenderDataType::Int);

  // TODO I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {

    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<uint32_t>& data) {
  checkType(RenderDataType::UInt);

  // TODO I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  bind();

  if (isSet()) {

    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");

  } else {
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::uvec2>& data) {
  checkType(RenderDataType::Vector2UInt);


  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
  } else {
    dataSize = data.size();
  }
}
void GLAttributeBuffer::setData(const std::vector<glm::uvec3>& data) {
  checkType(RenderDataType::Vector3UInt);

  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
  } else {
    dataSize = data.size();
  }
}

void GLAttributeBuffer::setData(const std::vector<glm::uvec4>& data) {
  checkType(RenderDataType::Vector4UInt);

  bind();

  if (isSet()) {
    if (static_cast<int64_t>(data.size()) != dataSize) exception("updated data must have same size");
  } else {
    dataSize = data.size();
  }
}

// get single data values

float GLAttributeBuffer::getData_float(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Float) exception("bad getData type");
  bind();
  float readValue = 777.;
  return readValue;
}
double GLAttributeBuffer::getData_double(size_t ind) { return getData_float(ind); }
glm::vec2 GLAttributeBuffer::getData_vec2(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  glm::vec2 readValue{777., 777.};
  return readValue;
}
glm::vec3 GLAttributeBuffer::getData_vec3(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  glm::vec3 readValue{777., 777., 777.};
  return readValue;
}
glm::vec4 GLAttributeBuffer::getData_vec4(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  glm::vec4 readValue{777., 777., 777., 777.};
  return readValue;
}
int GLAttributeBuffer::getData_int(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Int) exception("bad getData type");
  bind();
  int readValue = 777;
  return static_cast<int>(readValue);
}
uint32_t GLAttributeBuffer::getData_uint32(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::UInt) exception("bad getData type");
  bind();
  uint32_t readValue = 777;
  return readValue;
}
glm::uvec2 GLAttributeBuffer::getData_uvec2(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  glm::uvec2 readValue{777, 777};
  return readValue;
}
glm::uvec3 GLAttributeBuffer::getData_uvec3(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  glm::uvec3 readValue{777, 777, 777};
  return readValue;
}
glm::uvec4 GLAttributeBuffer::getData_uvec4(size_t ind) {
  if (!isSet() || ind >= static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  glm::uvec4 readValue{777, 777, 777, 777};
  return readValue;
}

// get ranges of data

std::vector<float> GLAttributeBuffer::getDataRange_float(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Float) exception("bad getData type");
  bind();
  std::vector<float> readValues(count);
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
  std::vector<glm::vec2> readValues(count);
  return readValues;
}
std::vector<glm::vec3> GLAttributeBuffer::getDataRange_vec3(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  std::vector<glm::vec3> readValues(count);
  return readValues;
}
std::vector<glm::vec4> GLAttributeBuffer::getDataRange_vec4(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  std::vector<glm::vec4> readValues(count);
  return readValues;
}
std::vector<int> GLAttributeBuffer::getDataRange_int(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Int) exception("bad getData type");
  bind();
  std::vector<int> readValues(count);

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
  std::vector<uint32_t> readValues(count);
  return readValues;
}
std::vector<glm::uvec2> GLAttributeBuffer::getDataRange_uvec2(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector2Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec2> readValues(count);
  return readValues;
}
std::vector<glm::uvec3> GLAttributeBuffer::getDataRange_uvec3(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector3Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec3> readValues(count);
  return readValues;
}
std::vector<glm::uvec4> GLAttributeBuffer::getDataRange_uvec4(size_t ind, size_t count) {
  if (!isSet() || ind + count > static_cast<size_t>(getDataSize())) exception("bad getData");
  if (getType() != RenderDataType::Vector4Float) exception("bad getData type");
  bind();
  std::vector<glm::uvec4> readValues(count);
  return readValues;
}

uint32_t GLAttributeBuffer::getNativeBufferID() { return 777; }

// =============================================================
// ==================== Texture buffer =========================
// =============================================================

// create a 1D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, const unsigned char* data)
    : TextureBuffer(1, format_, size1D) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, const float* data)
    : TextureBuffer(1, format_, size1D) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

// create a 2D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_,
                                 const unsigned char* data)
    : TextureBuffer(2, format_, sizeX_, sizeY_) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_, const float* data)
    : TextureBuffer(2, format_, sizeX_, sizeY_) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

GLTextureBuffer::~GLTextureBuffer() {}

void GLTextureBuffer::resize(unsigned int newLen) {

  TextureBuffer::resize(newLen);

  bind();
  if (dim == 1) {
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
  }
  checkGLError();
}

void GLTextureBuffer::setFilterMode(FilterMode newMode) {

  bind();

  if (dim == 1) {
    switch (newMode) {
    case FilterMode::Nearest:
      break;
    case FilterMode::Linear:
      break;
    }
  }
  if (dim == 2) {

    switch (newMode) {
    case FilterMode::Nearest:
      break;
    case FilterMode::Linear:
      break;
    }
  }
  checkGLError();
}

void* GLTextureBuffer::getNativeHandle() { return nullptr; }

std::vector<float> GLTextureBuffer::getDataScalar() {
  if (dimension(format) != 1) exception("called getDataScalar on texture which does not have a 1 dimensional format");
  std::vector<float> outData;
  outData.resize(getSizeX() * getSizeY());

  return outData;
}

std::vector<glm::vec2> GLTextureBuffer::getDataVector2() {
  if (dimension(format) != 2) exception("called getDataVector2 on texture which does not have a 2 dimensional format");

  std::vector<glm::vec2> outData;
  outData.resize(getSizeX() * getSizeY());

  return outData;
}

std::vector<glm::vec3> GLTextureBuffer::getDataVector3() {
  if (dimension(format) != 3) exception("called getDataVector3 on texture which does not have a 3 dimensional format");
  exception("not implemented");

  std::vector<glm::vec3> outData;
  outData.resize(getSizeX() * getSizeY());

  return outData;
}
void GLTextureBuffer::bind() {
  if (dim == 1) {
  }
  if (dim == 2) {
  }
  checkGLError();
}

// =============================================================
// ===================== Render buffer =========================
// =============================================================

GLRenderBuffer::GLRenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : RenderBuffer(type_, sizeX_, sizeY_) {
  checkGLError();
  resize(sizeX, sizeY);
}


GLRenderBuffer::~GLRenderBuffer() {}

void GLRenderBuffer::resize(unsigned int newX, unsigned int newY) {
  RenderBuffer::resize(newX, newY);
  bind();
  checkGLError();
}

void GLRenderBuffer::bind() { checkGLError(); }


// =============================================================
// ===================== Framebuffer ===========================
// =============================================================

GLFrameBuffer::GLFrameBuffer(unsigned int sizeX_, unsigned int sizeY_, bool isDefault) {
  sizeX = sizeX_;
  sizeY = sizeY_;
  if (isDefault) {
  } else {
  }
  checkGLError();
};

GLFrameBuffer::~GLFrameBuffer() {}

void GLFrameBuffer::bind() { checkGLError(); }

void GLFrameBuffer::addColorBuffer(std::shared_ptr<RenderBuffer> renderBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLRenderBuffer> renderBuffer = std::dynamic_pointer_cast<GLRenderBuffer>(renderBufferIn);
  if (!renderBuffer) exception("tried to bind to non-GL render buffer");

  renderBuffer->bind();
  bind();

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
  // if (depthTextureBuffer != nullptr) exception("OpenGL error: already bound to texture buffer");

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

  // Sanity checks
  // if (colorRenderBuffer != nullptr) exception("OpenGL error: already bound to render buffer");
  // if (colorTextureBuffer != nullptr) exception("OpenGL error: already bound to texture buffer");

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
  // if (depthTextureBuffer != nullptr) exception("OpenGL error: already bound to texture buffer");

  checkGLError();
  textureBuffersDepth.push_back(textureBuffer);
}

void GLFrameBuffer::setDrawBuffers() {
  bind();
  checkGLError();
}

bool GLFrameBuffer::bindForRendering() {
  bind();
  render::engine->currRenderFramebuffer = this;
  render::engine->setCurrentViewport({0, 0, view::bufferWidth, view::bufferHeight});
  checkGLError();
  return true;
}

void GLFrameBuffer::clear() {
  if (!bindForRendering()) return;
}

std::array<float, 4> GLFrameBuffer::readFloat4(int xPos, int yPos) {
  // Read from the buffer
  std::array<float, 4> result = {1., 2., 3., 4.};

  return result;
}

float GLFrameBuffer::readDepth(int xPos, int yPos) {
  // Read from the buffer
  float result = 0.5;
  return result;
}

std::vector<unsigned char> GLFrameBuffer::readBuffer() {
  bind();

  int w = getSizeX();
  int h = getSizeY();

  // Read from openGL
  size_t buffSize = w * h * 4;
  std::vector<unsigned char> buff(buffSize);

  return buff;
}

void GLFrameBuffer::blitTo(FrameBuffer* targetIn) {

  // it _better_ be a GL buffer
  GLFrameBuffer* target = dynamic_cast<GLFrameBuffer*>(targetIn);
  if (!target) exception("tried to blitTo() non-GL framebuffer");

  // target->bindForRendering();
  bindForRendering();
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
  setDataLocations();
}

GLCompiledProgram::~GLCompiledProgram() {}

void GLCompiledProgram::compileGLProgram(const std::vector<ShaderStageSpecification>& stages) {}

void GLCompiledProgram::setDataLocations() {
  // Uniforms
  for (GLShaderUniform& u : uniforms) {
  }

  // Attributes
  for (GLShaderAttribute& a : attributes) {
  }

  // Textures
  for (GLShaderTexture& t : textures) {
  }
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
  attributes.push_back(GLShaderAttribute{newAttribute.name, newAttribute.type, newAttribute.arrayCount, nullptr});
}

void GLCompiledProgram::addUniqueUniform(ShaderSpecUniform newUniform) {
  for (GLShaderUniform& u : uniforms) {
    if (u.name == newUniform.name) {

      // if it occurs twice, confirm that the occurences match
      if (u.type != newUniform.type) exception("uniform " + u.name + " appears twice in program with different types");

      return;
    }
  }
  uniforms.push_back(GLShaderUniform{newUniform.name, newUniform.type, false});
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
  textures.push_back(GLShaderTexture{newTexture.name, newTexture.dim, 777, false, nullptr, nullptr});
}


GLShaderProgram::GLShaderProgram(const std::shared_ptr<GLCompiledProgram>& compiledProgram_)
    : ShaderProgram(compiledProgram_->getDrawMode()), uniforms(compiledProgram_->getUniforms()),
      attributes(compiledProgram_->getAttributes()), textures(compiledProgram_->getTextures()),
      compiledProgram(compiledProgram_) {
  createBuffers(); // only handles texture & index things, attributes are lazily created
}


GLShaderProgram::~GLShaderProgram() {}

void GLShaderProgram::bindVAO() {}

void GLShaderProgram::createBuffers() {
  bindVAO();

  // Create an index buffer, if we're using one
  if (useIndex) {
  }

  // === Generate textures

  // Set indices sequentially
  uint32_t iTexture = 0;
  for (GLShaderTexture& t : textures) {
    t.index = iTexture++;
  }
}

void GLShaderProgram::setAttribute(std::string name, std::shared_ptr<AttributeBuffer> externalBuffer) {
  bindVAO();
  checkGLError();

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {

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

      a.buff->bind();

      assignBufferToVAO(a);
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

    switch (a.type) {
    case RenderDataType::Float:
      break;
    case RenderDataType::Int:
      break;
    case RenderDataType::UInt:
      break;
    case RenderDataType::Vector2Float:
      break;
    case RenderDataType::Vector3Float:
      break;
    case RenderDataType::Vector4Float:
      break;
    case RenderDataType::Vector2UInt:
      break;
    case RenderDataType::Vector3UInt:
      break;
    case RenderDataType::Vector4UInt:
      break;
    default:
      throw std::invalid_argument("Unrecognized GLShaderAttribute type");
      break;
    }
  }

  checkGLError();
}

void GLShaderProgram::createBuffer(GLShaderAttribute& a) {

  // generate the buffer if needed
  std::shared_ptr<AttributeBuffer> newBuff = glEngine->generateAttributeBuffer(a.type, a.arrayCount);
  std::shared_ptr<GLAttributeBuffer> engineNewBuff = std::dynamic_pointer_cast<GLAttributeBuffer>(newBuff);
  if (!engineNewBuff) throw std::invalid_argument("buffer type cast failed");
  a.buff = engineNewBuff;

  assignBufferToVAO(a);
}

void GLShaderProgram::ensureBufferExists(GLShaderAttribute& a) {
  if (!a.buff) {
    createBuffer(a);
  }
}

bool GLShaderProgram::hasUniform(std::string name) {
  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      return true;
    }
  }
  return false;
}

// Set an integer
void GLShaderProgram::setUniform(std::string name, int val) {

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Int) {
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
  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::UInt) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Float) {
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
void GLShaderProgram::setUniform(std::string name, float* val) {

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Matrix44Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector2Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector3Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector4Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector3Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector4Float) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector2UInt) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector3UInt) {
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

  for (GLShaderUniform& u : uniforms) {
    if (u.name == name) {
      if (u.type == RenderDataType::Vector4UInt) {
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
    if (a.name == name) {
      return true;
    }
  }
  return false;
}

bool GLShaderProgram::attributeIsSet(std::string name) {
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
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
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec3>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec4>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<float>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<double>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<int32_t>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<uint32_t>& data) {

  // pass-through to the buffer
  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      ensureBufferExists(a);
      a.buff->setData(data);
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

bool GLShaderProgram::hasTexture(std::string name) {
  for (GLShaderTexture& t : textures) {
    if (t.name == name) {
      return true;
    }
  }
  return false;
}

bool GLShaderProgram::textureIsSet(std::string name) {
  for (GLShaderTexture& t : textures) {
    if (t.name == name) {
      return t.isSet;
    }
  }
  return false;
}

void GLShaderProgram::setTexture1D(std::string name, unsigned char* texData, unsigned int length) {
  throw std::invalid_argument("This code hasn't been testded yet.");

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name) continue;

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
    if (t.name != name) continue;

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
    } else {
    }

    // Use mip maps
    if (useMipMap) {
    } else {
    }

    t.isSet = true;
    return;
  }

  throw std::invalid_argument("No texture with name " + name);
}

void GLShaderProgram::setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer) {

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name) continue;

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

  // Find the right texture
  for (GLShaderTexture& t : textures) {
    if (t.name != name) continue;

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

void GLShaderProgram::setIndex(std::vector<std::array<unsigned int, 3>>& indices) {
  if (!useIndex) {
    throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed "
                                "drawing");
  }

  // Reshape the vector
  // Right now, the data is probably laid out in this form already... but let's
  // not be overly clever and just reshape it.
  unsigned int* rawData = new unsigned int[3 * indices.size()];
  indexSize = 3 * indices.size();
  for (unsigned int i = 0; i < indices.size(); i++) {
    rawData[3 * i + 0] = static_cast<float>(indices[i][0]);
    rawData[3 * i + 1] = static_cast<float>(indices[i][1]);
    rawData[3 * i + 2] = static_cast<float>(indices[i][2]);
  }

  delete[] rawData;
}

void GLShaderProgram::setIndex(std::vector<glm::uvec3>& indices) {
  if (!useIndex) {
    throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed "
                                "drawing");
  }

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
    unsigned int bigThresh = 99999999;
    for (unsigned int x : indices) {
      if (x > bigThresh) {
        throw std::invalid_argument("An unusual index was passed, but setPrimitiveRestartIndex() has not been called.");
      }
    }
  }
  indexSize = indices.size();
}

// Check that uniforms and attributes are all set and of consistent size
void GLShaderProgram::validateData() {
  // Check uniforms
  for (GLShaderUniform& u : uniforms) {
    if (!u.isSet) {
      throw std::invalid_argument("Uniform " + u.name + " has not been set");
    }
  }

  // Check attributes
  int64_t attributeSize = -1;
  for (GLShaderAttribute a : attributes) {
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
    // Point the uniform at this texture

    // Bind to the texture buffer
    switch (t.dim) {
    case 1:
      break;
    case 2:
      break;
    }

    t.textureBuffer->bind();
  }
}

void GLShaderProgram::draw() {
  validateData();

  if (usePrimitiveRestart) {
  }

  activateTextures();

  switch (drawMode) {
  case DrawMode::Points:
    break;
  case DrawMode::Triangles:
    break;
  case DrawMode::Lines:
    break;
  case DrawMode::TrianglesAdjacency:
    break;
  case DrawMode::LinesAdjacency:
    break;
  case DrawMode::IndexedLines:
    break;
  case DrawMode::IndexedLineStrip:
    break;
  case DrawMode::IndexedLinesAdjacency:
    break;
  case DrawMode::IndexedLineStripAdjacency:
    break;
  case DrawMode::IndexedTriangles:
    break;
  }

  if (usePrimitiveRestart) {
  }

  checkGLError();
}

MockGLEngine::MockGLEngine() {}

void MockGLEngine::initialize() {

  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "Backend: openGL_mock" << std::endl;
  }

  GLFrameBuffer* glScreenBuffer = new GLFrameBuffer(view::bufferWidth, view::bufferHeight, true);
  displayBuffer.reset(glScreenBuffer);


  // normally we get initial values for the buffer size from the window framework,
  // with the mock backend we we need to manually set them to some sane value
  view::bufferWidth = view::windowWidth;
  view::bufferHeight = view::windowHeight;

  updateWindowSize();

  populateDefaultShadersAndRules();
}

void MockGLEngine::initializeImGui() {
  ImGui::CreateContext(); // must call once at start
  configureImGui();
}

void MockGLEngine::shutdownImGui() { ImGui::DestroyContext(); }

void MockGLEngine::swapDisplayBuffers() {}

std::vector<unsigned char> MockGLEngine::readDisplayBuffer() {
  // Get buffer size
  int w = view::bufferWidth;
  int h = view::bufferHeight;

  // Read from openGL
  size_t buffSize = w * h * 4;
  std::vector<unsigned char> buff(buffSize, 0);
  return buff;
}


void MockGLEngine::checkError(bool fatal) { checkGLError(fatal); }

void MockGLEngine::makeContextCurrent() {}

void MockGLEngine::focusWindow() {}

void MockGLEngine::showWindow() {}

void MockGLEngine::hideWindow() {}

void MockGLEngine::updateWindowSize(bool force) {

  // this silly code mimicks the gl backend version, but it is important that we preserve
  // the view::bufferWidth, etc, otherwise it is impossible to manually set the window size
  // in the mock backend (which appears in unit tests, etc)
  int newBufferWidth = view::bufferWidth;
  int newBufferHeight = view::bufferHeight;
  int newWindowWidth = view::windowWidth;
  int newWindowHeight = view::windowHeight;

  if (force || newBufferWidth != view::bufferWidth || newBufferHeight != view::bufferHeight ||
      newWindowHeight != view::windowHeight || newWindowWidth != view::windowWidth) {
    // Basically a resize callback
    requestRedraw();
    view::bufferWidth = newBufferWidth;
    view::bufferHeight = newBufferHeight;
    view::windowWidth = newWindowWidth;
    view::windowHeight = newWindowHeight;
  }
}


void MockGLEngine::applyWindowSize() { updateWindowSize(true); }

void MockGLEngine::setWindowResizable(bool newVal) {}

bool MockGLEngine::getWindowResizable() { return true; }

std::tuple<int, int> MockGLEngine::getWindowPos() {
  int x = 20;
  int y = 40;
  return std::tuple<int, int>{x, y};
}

bool MockGLEngine::windowRequestsClose() { return false; }

void MockGLEngine::pollEvents() {}

bool MockGLEngine::isKeyPressed(char c) { return false; }

void MockGLEngine::ImGuiNewFrame() {

  // ImGUI seems to crash without a backend if we don't do this
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize.x = view::bufferWidth;
  io.DisplaySize.y = view::bufferHeight;

  ImGui::NewFrame();
}

void MockGLEngine::ImGuiRender() { ImGui::Render(); }

void MockGLEngine::setDepthMode(DepthMode newMode) {}

void MockGLEngine::setBlendMode(BlendMode newMode) {}

void MockGLEngine::setColorMask(std::array<bool, 4> mask) {}

void MockGLEngine::setBackfaceCull(bool newVal) {}

std::string MockGLEngine::getClipboardText() {
  std::string clipboardData = "";
  return clipboardData;
}

void MockGLEngine::setClipboardText(std::string text) {}

void MockGLEngine::applyTransparencySettings() {}

void MockGLEngine::setFrontFaceCCW(bool newVal) {
  if (newVal == frontFaceCCW) return;
  frontFaceCCW = newVal;
}

// == Factories


std::shared_ptr<AttributeBuffer> MockGLEngine::generateAttributeBuffer(RenderDataType dataType_, int arrayCount_) {
  GLAttributeBuffer* newA = new GLAttributeBuffer(dataType_, arrayCount_);
  return std::shared_ptr<AttributeBuffer>(newA);
}

std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                                   const unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}

std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                                   const float* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                                   unsigned int sizeY_, const unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, sizeX_, sizeY_, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                                   unsigned int sizeY_, const float* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, sizeX_, sizeY_, data);
  return std::shared_ptr<TextureBuffer>(newT);
}


std::shared_ptr<RenderBuffer> MockGLEngine::generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                                 unsigned int sizeY_) {
  GLRenderBuffer* newR = new GLRenderBuffer(type, sizeX_, sizeY_);
  return std::shared_ptr<RenderBuffer>(newR);
}

std::shared_ptr<FrameBuffer> MockGLEngine::generateFrameBuffer(unsigned int sizeX_, unsigned int sizeY_) {
  GLFrameBuffer* newF = new GLFrameBuffer(sizeX_, sizeY_);
  return std::shared_ptr<FrameBuffer>(newF);
}

std::string MockGLEngine::programKeyFromRules(const std::string& programName, const std::vector<std::string>& rules,
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


std::shared_ptr<GLCompiledProgram> MockGLEngine::getCompiledProgram(const std::string& programName,
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

std::shared_ptr<ShaderProgram> MockGLEngine::requestShader(const std::string& programName,
                                                           const std::vector<std::string>& customRules,
                                                           ShaderReplacementDefaults defaults) {
  GLShaderProgram* newP = new GLShaderProgram(getCompiledProgram(programName, customRules, defaults));
  return std::shared_ptr<ShaderProgram>(newP);
}

void MockGLEngine::registerShaderProgram(const std::string& name, const std::vector<ShaderStageSpecification>& spec,
                                         const DrawMode& dm) {
  registeredShaderPrograms.insert({name, {spec, dm}});
}

void MockGLEngine::registerShaderRule(const std::string& name, const ShaderReplacementRule& rule) {
  registeredShaderRules.insert({name, rule});
}


void MockGLEngine::populateDefaultShadersAndRules() {
  using namespace backend_openGL3_glfw;

  // WARNING: duplicated from gl_engine.cpp

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


void MockGLEngine::createSlicePlaneFliterRule(std::string uniquePostfix) {
  using namespace backend_openGL3_glfw;
  registeredShaderRules.insert({"SLICE_PLANE_CULL_" + uniquePostfix, generateSlicePlaneRule(uniquePostfix)});
}

} // namespace backend_openGL_mock
} // namespace render
} // namespace polyscope

#else

#include <stdexcept>

#include "polyscope/messages.h"

namespace polyscope {
namespace render {
namespace backend_openGL_mock {
void initializeRenderEngine() { exception("Polyscope was not compiled with support for backend: openGL_mock"); }
} // namespace backend_openGL_mock
} // namespace render
} // namespace polyscope

#endif

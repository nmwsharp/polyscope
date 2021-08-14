// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
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


#include "stb_image.h"

namespace polyscope {
namespace render {
namespace backend_openGL_mock {


MockGLEngine* glEngine = nullptr; // alias for engine pointer

void initializeRenderEngine() {
  glEngine = new MockGLEngine();
  glEngine->initialize();
  engine = glEngine;
  engine->allocateGlobalBuffersAndPrograms();
}

// == Map enums to native values

void checkGLError(bool fatal = true) {}

// =============================================================
// ==================== Texture buffer =========================
// =============================================================

// create a 1D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, unsigned char* data)
    : TextureBuffer(1, format_, size1D) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int size1D, float* data)
    : TextureBuffer(1, format_, size1D) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

// create a 2D texture from data
GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_, unsigned char* data)
    : TextureBuffer(2, format_, sizeX_, sizeY_) {

  checkGLError();

  setFilterMode(FilterMode::Nearest);
}

GLTextureBuffer::GLTextureBuffer(TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_, float* data)
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
    throw std::runtime_error("OpenGL error: called 1D resize on 2D texture");
  }
  checkGLError();
}

void GLTextureBuffer::resize(unsigned int newX, unsigned int newY) {

  TextureBuffer::resize(newX, newY);

  bind();
  if (dim == 1) {
    throw std::runtime_error("OpenGL error: called 2D resize on 1D texture");
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
  if (dimension(format) != 1)
    throw std::runtime_error("called getDataScalar on texture which does not have a 1 dimensional format");
  std::vector<float> outData;
  outData.resize(getSizeX() * getSizeY());

  return outData;
}

std::vector<glm::vec2> GLTextureBuffer::getDataVector2() {
  if (dimension(format) != 2)
    throw std::runtime_error("called getDataVector2 on texture which does not have a 2 dimensional format");

  std::vector<glm::vec2> outData;
  outData.resize(getSizeX() * getSizeY());

  return outData;
}

std::vector<glm::vec3> GLTextureBuffer::getDataVector3() {
  if (dimension(format) != 3)
    throw std::runtime_error("called getDataVector3 on texture which does not have a 3 dimensional format");
  throw std::runtime_error("not implemented");

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
  if (!renderBuffer) throw std::runtime_error("tried to bind to non-GL render buffer");

  renderBuffer->bind();
  bind();

  checkGLError();
  renderBuffersColor.push_back(renderBuffer);
  nColorBuffers++;
}

void GLFrameBuffer::addDepthBuffer(std::shared_ptr<RenderBuffer> renderBufferIn) {
  // it _better_ be a GL buffer
  std::shared_ptr<GLRenderBuffer> renderBuffer = std::dynamic_pointer_cast<GLRenderBuffer>(renderBufferIn);
  if (!renderBuffer) throw std::runtime_error("tried to bind to non-GL render buffer");

  renderBuffer->bind();
  bind();

  // Sanity checks
  // if (depthRenderBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to render buffer");
  // if (depthTextureBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to texture buffer");

  checkGLError();
  renderBuffersDepth.push_back(renderBuffer);
}

void GLFrameBuffer::addColorBuffer(std::shared_ptr<TextureBuffer> textureBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLTextureBuffer> textureBuffer = std::dynamic_pointer_cast<GLTextureBuffer>(textureBufferIn);
  if (!textureBuffer) throw std::runtime_error("tried to bind to non-GL texture buffer");

  textureBuffer->bind();
  bind();
  checkGLError();

  // Sanity checks
  // if (colorRenderBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to render buffer");
  // if (colorTextureBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to texture buffer");

  checkGLError();
  textureBuffersColor.push_back(textureBuffer);
  nColorBuffers++;
}

void GLFrameBuffer::addDepthBuffer(std::shared_ptr<TextureBuffer> textureBufferIn) {

  // it _better_ be a GL buffer
  std::shared_ptr<GLTextureBuffer> textureBuffer = std::dynamic_pointer_cast<GLTextureBuffer>(textureBufferIn);
  if (!textureBuffer) throw std::runtime_error("tried to bind to non-GL texture buffer");

  textureBuffer->bind();
  bind();
  checkGLError();

  // Sanity checks
  // if (depthRenderBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to render buffer");
  // if (depthTextureBuffer != nullptr) throw std::runtime_error("OpenGL error: already bound to texture buffer");

  checkGLError();
  textureBuffersDepth.push_back(textureBuffer);
}

void GLFrameBuffer::setDrawBuffers() {
  bind();
  checkGLError();
}

bool GLFrameBuffer::bindForRendering() {
  bind();
  render::engine->setCurrentViewport({0, 0, 400, 600});
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
  if (!target) throw std::runtime_error("tried to blitTo() non-GL framebuffer");

  // target->bindForRendering();
  bindForRendering();
  checkGLError();
}

// =============================================================
// ==================  Shader Program  =========================
// =============================================================

GLShaderProgram::GLShaderProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm)
    : ShaderProgram(stages, dm) {


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
  createBuffers();
  checkGLError();
}

GLShaderProgram::~GLShaderProgram() {}

void GLShaderProgram::addUniqueAttribute(ShaderSpecAttribute newAttribute) {
  for (GLShaderAttribute& a : attributes) {
    if (a.name == newAttribute.name && a.type == newAttribute.type) {
      return;
    }
  }
  attributes.push_back(GLShaderAttribute{newAttribute.name, newAttribute.type, newAttribute.arrayCount, -1, 777, 777});
}

void GLShaderProgram::addUniqueUniform(ShaderSpecUniform newUniform) {
  for (GLShaderUniform& u : uniforms) {
    if (u.name == newUniform.name && u.type == newUniform.type) {
      return;
    }
  }
  uniforms.push_back(GLShaderUniform{newUniform.name, newUniform.type, false, 777});
}

void GLShaderProgram::addUniqueTexture(ShaderSpecTexture newTexture) {
  for (GLShaderTexture& t : textures) {
    if (t.name == newTexture.name && t.dim == newTexture.dim) {
      return;
    }
  }
  textures.push_back(GLShaderTexture{newTexture.name, newTexture.dim, 777, false, nullptr, nullptr, 777});
}


void GLShaderProgram::deleteAttributeBuffer(GLShaderAttribute& attribute) {}

void GLShaderProgram::compileGLProgram(const std::vector<ShaderStageSpecification>& stages) { checkGLError(); }

void GLShaderProgram::setDataLocations() {

  // Uniforms
  unsigned int iLoc = 0;
  for (GLShaderUniform& u : uniforms) {
    u.location = iLoc++;
    if (u.location == -1) throw std::runtime_error("failed to get location for uniform " + u.name);
  }

  // Attributes
  for (GLShaderAttribute& a : attributes) {
    a.location = iLoc++;
    if (a.location == -1) throw std::runtime_error("failed to get location for attribute " + a.name);
  }

  // Textures
  for (GLShaderTexture& t : textures) {
    t.location = iLoc++;
    if (t.location == -1) throw std::runtime_error("failed to get location for texture " + t.name);
  }

  checkGLError();
}

void GLShaderProgram::createBuffers() {
  // Create buffers for each attributes
  for (GLShaderAttribute& a : attributes) {

    // Choose the correct type for the buffer
    for (int iArrInd = 0; iArrInd < a.arrayCount; iArrInd++) {
      switch (a.type) {
      case DataType::Float:
        break;
      case DataType::Int:
        break;
      case DataType::UInt:
        break;
      case DataType::Vector2Float:
        break;
      case DataType::Vector3Float:
        break;
      case DataType::Vector4Float:
        break;
      default:
        throw std::invalid_argument("Unrecognized GLShaderAttribute type");
        break;
      }
    }
  }

  // === Generate textures

  // Set indices sequentially
  for (unsigned int iTexture = 0; iTexture < textures.size(); iTexture++) {
    GLShaderTexture& t = textures[iTexture];
    t.index = iTexture;
  }

  checkGLError();
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
      if (u.type == DataType::Int) {
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
      if (u.type == DataType::UInt) {
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
      if (u.type == DataType::Float) {
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
      if (u.type == DataType::Float) {
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
      if (u.type == DataType::Matrix44Float) {
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
      if (u.type == DataType::Vector2Float) {
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
      if (u.type == DataType::Vector3Float) {
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
      if (u.type == DataType::Vector4Float) {
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
      if (u.type == DataType::Vector3Float) {
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
      if (u.type == DataType::Vector4Float) {
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
      return a.dataSize != -1;
    }
  }
  return false;
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec2>& data, bool update, int offset,
                                   int size) {
  // Reshape the vector
  // Right now, the data is probably laid out in this form already... but let's
  // not be overly clever and just reshape it.
  std::vector<float> rawData(2 * data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    rawData[2 * i + 0] = static_cast<float>(data[i].x);
    rawData[2 * i + 1] = static_cast<float>(data[i].y);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::Vector2Float) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= 2 * sizeof(float);
          if (size == -1)
            size = 2 * a.dataSize * sizeof(float);
          else
            size *= 2 * sizeof(float);
        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<int>(DataType::Vector2Float)));
      }
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec3>& data, bool update, int offset,
                                   int size) {
  // Reshape the vector
  // Right now, the data is probably laid out in this form already... but let's
  // not be overly clever and just reshape it.
  std::vector<float> rawData(3 * data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    rawData[3 * i + 0] = static_cast<float>(data[i].x);
    rawData[3 * i + 1] = static_cast<float>(data[i].y);
    rawData[3 * i + 2] = static_cast<float>(data[i].z);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::Vector3Float) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= 3 * sizeof(float);
          if (size == -1)
            size = 3 * a.dataSize * sizeof(float);
          else
            size *= 3 * sizeof(float);

        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<int>(DataType::Vector3Float)));
      }
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<glm::vec4>& data, bool update, int offset,
                                   int size) {
  // Reshape the vector
  // Right now, the data is probably laid out in this form already... but let's
  // not be overly clever and just reshape it.
  std::vector<float> rawData(4 * data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    rawData[4 * i + 0] = static_cast<float>(data[i].x);
    rawData[4 * i + 1] = static_cast<float>(data[i].y);
    rawData[4 * i + 2] = static_cast<float>(data[i].z);
    rawData[4 * i + 3] = static_cast<float>(data[i].w);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::Vector4Float) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= 4 * sizeof(float);
          if (size == -1)
            size = 4 * a.dataSize * sizeof(float);
          else
            size *= 4 * sizeof(float);

        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<int>(DataType::Vector4Float)));
      }
      return;
    }
  }

  throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<double>& data, bool update, int offset,
                                   int size) {
  // Convert input data to floats
  std::vector<float> floatData(data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    floatData[i] = static_cast<float>(data[i]);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::Float) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= sizeof(float);
          if (size == -1)
            size = a.dataSize * sizeof(float);
          else
            size *= sizeof(float);

        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<float>(DataType::Float)));
      }
      return;
    }
  }

  throw std::invalid_argument("No attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<int>& data, bool update, int offset, int size) {
  // FIXME I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  // Convert data to GL_INT (probably does nothing)
  std::vector<int> intData(data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    intData[i] = static_cast<int>(data[i]);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::Int) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= sizeof(int);
          if (size == -1)
            size = a.dataSize * sizeof(int);
          else
            size *= sizeof(int);

        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<int>(DataType::Int)));
      }
      return;
    }
  }

  throw std::invalid_argument("No attribute with name " + name);
}

void GLShaderProgram::setAttribute(std::string name, const std::vector<uint32_t>& data, bool update, int offset,
                                   int size) {
  // FIXME I've seen strange bugs when using int's in shaders. Need to figure
  // out it it's my shaders or something wrong with this function

  // Convert data to GL_UINT (probably does nothing)
  std::vector<unsigned int> intData(data.size());
  for (unsigned int i = 0; i < data.size(); i++) {
    intData[i] = static_cast<unsigned int>(data[i]);
  }

  for (GLShaderAttribute& a : attributes) {
    if (a.name == name) {
      if (a.type == DataType::UInt) {
        if (update) {
          // TODO: Allow modifications to non-contiguous memory
          offset *= sizeof(unsigned int);
          if (size == -1)
            size = a.dataSize * sizeof(unsigned int);
          else
            size *= sizeof(unsigned int);

        } else {
          a.dataSize = data.size();
        }
      } else {
        throw std::invalid_argument("Tried to set GLShaderAttribute named " + name +
                                    " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) +
                                    "  Attempted type: " + std::to_string(static_cast<int>(DataType::UInt)));
      }
      return;
    }
  }

  throw std::invalid_argument("No attribute with name " + name);
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
  long int attributeSize = -1;
  for (GLShaderAttribute a : attributes) {
    if (a.dataSize < 0) {
      throw std::invalid_argument("Attribute " + a.name + " has not been set");
    }
    if (attributeSize == -1) { // first one we've seen
      attributeSize = a.dataSize / a.arrayCount;
    } else { // not the first one we've seen
      if (a.dataSize / a.arrayCount != attributeSize) {
        throw std::invalid_argument("Attributes have inconsistent size. One attribute has size " +
                                    std::to_string(attributeSize) + " and " + a.name + " has size " +
                                    std::to_string(a.dataSize));
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
    throw std::runtime_error("setPrimitiveRestartIndex() called, but draw mode does not support restart indices.");
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

  updateWindowSize();

  populateDefaultShadersAndRules();
}

void MockGLEngine::initializeImGui() {
  ImGui::CreateContext(); // must call once at start
  configureImGui();
}

void MockGLEngine::shutdownImGui() { ImGui::DestroyContext(); }

void MockGLEngine::bindDisplay() {}


void MockGLEngine::clearDisplay() { bindDisplay(); }

void MockGLEngine::swapDisplayBuffers() {}

std::vector<unsigned char> MockGLEngine::readDisplayBuffer() {
  // Get buffer size
  int w = 400;
  int h = 600;

  // Read from openGL
  size_t buffSize = w * h * 4;
  std::vector<unsigned char> buff(buffSize, 0);
  return buff;
}


void MockGLEngine::checkError(bool fatal) { checkGLError(fatal); }

void MockGLEngine::makeContextCurrent() {}

void MockGLEngine::showWindow() {}

void MockGLEngine::hideWindow() {}

void MockGLEngine::updateWindowSize(bool force) {
  int newBufferWidth = 400;
  int newBufferHeight = 600;
  int newWindowWidth = 400;
  int newWindowHeight = 600;
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

// == Factories
std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                                   unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}

std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                                   float* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, size1D, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                                   unsigned int sizeY_, unsigned char* data) {
  GLTextureBuffer* newT = new GLTextureBuffer(format, sizeX_, sizeY_, data);
  return std::shared_ptr<TextureBuffer>(newT);
}
std::shared_ptr<TextureBuffer> MockGLEngine::generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                                   unsigned int sizeY_, float* data) {
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

std::shared_ptr<ShaderProgram> MockGLEngine::generateShaderProgram(const std::vector<ShaderStageSpecification>& stages,
                                                                   DrawMode dm) {
  GLShaderProgram* newP = new GLShaderProgram(stages, dm);
  return std::shared_ptr<ShaderProgram>(newP);
}

std::shared_ptr<ShaderProgram> MockGLEngine::requestShader(const std::string& programName,
                                                           const std::vector<std::string>& customRules,
                                                           ShaderReplacementDefaults defaults) {

  // Get the program
  if (registeredShaderPrograms.find(programName) == registeredShaderPrograms.end()) {
    throw std::runtime_error("No shader program with name [" + programName + "] registered.");
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

  // Get the rules
  std::vector<ShaderReplacementRule> rules;
  for (const std::string& ruleName : fullCustomRules) {
    if (registeredShaderRules.find(ruleName) == registeredShaderRules.end()) {
      throw std::runtime_error("No shader replacement rule with name [" + ruleName + "] registered.");
    }
    ShaderReplacementRule& thisRule = registeredShaderRules[ruleName];
    rules.push_back(thisRule);
  }

  std::vector<ShaderStageSpecification> updatedStages = applyShaderReplacements(stages, rules);
  return generateShaderProgram(updatedStages, dm);
}

void MockGLEngine::applyTransparencySettings() {}

void MockGLEngine::setFrontFaceCCW(bool newVal) {
  if (newVal == frontFaceCCW) return;
  frontFaceCCW = newVal;
}

void MockGLEngine::populateDefaultShadersAndRules() {
  using namespace backend_openGL3_glfw;

  // WARNING: duplicated from gl_engine.cpp

  // clang-format off

  // == Load general base shaders
  registeredShaderPrograms.insert({"MESH", {{FLEX_MESH_VERT_SHADER, FLEX_MESH_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"RAYCAST_SPHERE", {{FLEX_SPHERE_VERT_SHADER, FLEX_SPHERE_GEOM_SHADER, FLEX_SPHERE_FRAG_SHADER}, DrawMode::Points}});
  registeredShaderPrograms.insert({"RAYCAST_VECTOR", {{FLEX_VECTOR_VERT_SHADER, FLEX_VECTOR_GEOM_SHADER, FLEX_VECTOR_FRAG_SHADER}, DrawMode::Points}});
  registeredShaderPrograms.insert({"RAYCAST_CYLINDER", {{FLEX_CYLINDER_VERT_SHADER, FLEX_CYLINDER_GEOM_SHADER, FLEX_CYLINDER_FRAG_SHADER}, DrawMode::Points}});
  registeredShaderPrograms.insert({"HISTOGRAM", {{HISTOGRAM_VERT_SHADER, HISTOGRAM_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"GROUND_PLANE_TILE", {{GROUND_PLANE_VERT_SHADER, GROUND_PLANE_TILE_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"GROUND_PLANE_TILE_REFLECT", {{GROUND_PLANE_VERT_SHADER, GROUND_PLANE_TILE_REFLECT_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"GROUND_PLANE_SHADOW", {{GROUND_PLANE_VERT_SHADER, GROUND_PLANE_SHADOW_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"MAP_LIGHT", {{TEXTURE_DRAW_VERT_SHADER, MAP_LIGHT_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"RIBBON", {{RIBBON_VERT_SHADER, RIBBON_GEOM_SHADER, RIBBON_FRAG_SHADER}, DrawMode::IndexedLineStripAdjacency}});
  registeredShaderPrograms.insert({"SLICE_PLANE", {{SLICE_PLANE_VERT_SHADER, SLICE_PLANE_FRAG_SHADER}, DrawMode::Triangles}});

  registeredShaderPrograms.insert({"TEXTURE_DRAW_PLAIN", {{TEXTURE_DRAW_VERT_SHADER, PLAIN_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"TEXTURE_DRAW_DOT3", {{TEXTURE_DRAW_VERT_SHADER, DOT3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"TEXTURE_DRAW_MAP3", {{TEXTURE_DRAW_VERT_SHADER, MAP3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"TEXTURE_DRAW_SPHEREBG", {{SPHEREBG_DRAW_VERT_SHADER, SPHEREBG_DRAW_FRAG_SHADER}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"COMPOSITE_PEEL", {{TEXTURE_DRAW_VERT_SHADER, COMPOSITE_PEEL}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"DEPTH_COPY", {{TEXTURE_DRAW_VERT_SHADER, DEPTH_COPY}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"DEPTH_TO_MASK", {{TEXTURE_DRAW_VERT_SHADER, DEPTH_TO_MASK}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"SCALAR_TEXTURE_COLORMAP", {{TEXTURE_DRAW_VERT_SHADER, SCALAR_TEXTURE_COLORMAP}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"BLUR_RGB", {{TEXTURE_DRAW_VERT_SHADER, BLUR_RGB}, DrawMode::Triangles}});
  registeredShaderPrograms.insert({"TRANSFORMATION_GIZMO_ROT", {{TRANSFORMATION_GIZMO_ROT_VERT, TRANSFORMATION_GIZMO_ROT_FRAG}, DrawMode::Triangles}});


  // === Load rules

  // Utilitiy rules
  registeredShaderRules.insert({"GLSL_VERSION", GLSL_VERSION});
  registeredShaderRules.insert({"GLOBAL_FRAGMENT_FILTER", GLOBAL_FRAGMENT_FILTER});
  registeredShaderRules.insert({"DOWNSAMPLE_RESOLVE_1", DOWNSAMPLE_RESOLVE_1});
  registeredShaderRules.insert({"DOWNSAMPLE_RESOLVE_2", DOWNSAMPLE_RESOLVE_2});
  registeredShaderRules.insert({"DOWNSAMPLE_RESOLVE_3", DOWNSAMPLE_RESOLVE_3});
  registeredShaderRules.insert({"DOWNSAMPLE_RESOLVE_4", DOWNSAMPLE_RESOLVE_4});
  
  registeredShaderRules.insert({"TRANSPARENCY_STRUCTURE", TRANSPARENCY_STRUCTURE});
  registeredShaderRules.insert({"TRANSPARENCY_RESOLVE_SIMPLE", TRANSPARENCY_RESOLVE_SIMPLE});
  registeredShaderRules.insert({"TRANSPARENCY_PEEL_STRUCTURE", TRANSPARENCY_PEEL_STRUCTURE});
  registeredShaderRules.insert({"TRANSPARENCY_PEEL_GROUND", TRANSPARENCY_PEEL_GROUND});
  
  registeredShaderRules.insert({"GENERATE_VIEW_POS", GENERATE_VIEW_POS});
  registeredShaderRules.insert({"CULL_POS_FROM_VIEW", CULL_POS_FROM_VIEW});
  //registeredShaderRules.insert({"CULL_POS_FROM_ATTR", CULL_POS_FROM_ATTR});

  // Lighting and shading things
  registeredShaderRules.insert({"LIGHT_MATCAP", LIGHT_MATCAP});
  registeredShaderRules.insert({"LIGHT_PASSTHRU", LIGHT_PASSTHRU});
  registeredShaderRules.insert({"SHADE_BASECOLOR", SHADE_BASECOLOR});
  registeredShaderRules.insert({"SHADE_COLOR", SHADE_COLOR});
  registeredShaderRules.insert({"SHADE_COLORMAP_VALUE", SHADE_COLORMAP_VALUE});
  registeredShaderRules.insert({"SHADE_COLORMAP_ANGULAR2", SHADE_COLORMAP_ANGULAR2});
  registeredShaderRules.insert({"SHADE_GRID_VALUE2", SHADE_GRID_VALUE2});
  registeredShaderRules.insert({"SHADE_CHECKER_VALUE2", SHADE_CHECKER_VALUE2});
  registeredShaderRules.insert({"SHADEVALUE_MAG_VALUE2", SHADEVALUE_MAG_VALUE2});
  registeredShaderRules.insert({"ISOLINE_STRIPE_VALUECOLOR", ISOLINE_STRIPE_VALUECOLOR});
  registeredShaderRules.insert({"CHECKER_VALUE2COLOR", CHECKER_VALUE2COLOR});
  
  // mesh things
  registeredShaderRules.insert({"MESH_WIREFRAME", MESH_WIREFRAME});
  registeredShaderRules.insert({"MESH_BACKFACE_NORMAL_FLIP", MESH_BACKFACE_NORMAL_FLIP});
  registeredShaderRules.insert({"MESH_BACKFACE_DARKEN", MESH_BACKFACE_DARKEN});
  registeredShaderRules.insert({"MESH_PROPAGATE_VALUE", MESH_PROPAGATE_VALUE});
  registeredShaderRules.insert({"MESH_PROPAGATE_VALUE2", MESH_PROPAGATE_VALUE2});
  registeredShaderRules.insert({"MESH_PROPAGATE_COLOR", MESH_PROPAGATE_COLOR});
  registeredShaderRules.insert({"MESH_PROPAGATE_HALFEDGE_VALUE", MESH_PROPAGATE_HALFEDGE_VALUE});
  registeredShaderRules.insert({"MESH_PROPAGATE_CULLPOS", MESH_PROPAGATE_CULLPOS});
  registeredShaderRules.insert({"MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE", MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE});
  registeredShaderRules.insert({"MESH_PROPAGATE_PICK", MESH_PROPAGATE_PICK});

  // sphere things
  registeredShaderRules.insert({"SPHERE_PROPAGATE_VALUE", SPHERE_PROPAGATE_VALUE});
  registeredShaderRules.insert({"SPHERE_PROPAGATE_VALUE2", SPHERE_PROPAGATE_VALUE2});
  registeredShaderRules.insert({"SPHERE_PROPAGATE_COLOR", SPHERE_PROPAGATE_COLOR});
  registeredShaderRules.insert({"SPHERE_CULLPOS_FROM_CENTER", SPHERE_CULLPOS_FROM_CENTER});
  registeredShaderRules.insert({"SPHERE_VARIABLE_SIZE", SPHERE_VARIABLE_SIZE});

  // vector things
  registeredShaderRules.insert({"VECTOR_PROPAGATE_COLOR", VECTOR_PROPAGATE_COLOR});
  registeredShaderRules.insert({"VECTOR_CULLPOS_FROM_TAIL", VECTOR_CULLPOS_FROM_TAIL});
  registeredShaderRules.insert({"TRANSFORMATION_GIZMO_VEC", TRANSFORMATION_GIZMO_VEC});

  // cylinder things
  registeredShaderRules.insert({"CYLINDER_PROPAGATE_VALUE", CYLINDER_PROPAGATE_VALUE});
  registeredShaderRules.insert({"CYLINDER_PROPAGATE_BLEND_VALUE", CYLINDER_PROPAGATE_BLEND_VALUE});
  registeredShaderRules.insert({"CYLINDER_PROPAGATE_COLOR", CYLINDER_PROPAGATE_COLOR});
  registeredShaderRules.insert({"CYLINDER_PROPAGATE_BLEND_COLOR", CYLINDER_PROPAGATE_BLEND_COLOR});
  registeredShaderRules.insert({"CYLINDER_PROPAGATE_PICK", CYLINDER_PROPAGATE_PICK});
  registeredShaderRules.insert({"CYLINDER_CULLPOS_FROM_MID", CYLINDER_CULLPOS_FROM_MID});

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

namespace polyscope {
namespace render {
namespace backend_openGL_mock {
void initializeRenderEngine() {
  throw std::runtime_error("Polyscope was not compiled with support for backend: openGL_mock");
}
} // namespace backend_openGL_mock
} // namespace render
} // namespace polyscope

#endif

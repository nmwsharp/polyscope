// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

//#include "polyscope/colors.h"
//#include "polyscope/gl/color_maps.h"
//#include "polyscope/gl/shaders.h"
#include "polyscope/view.h"

// Make syntax  nicer like this, but we lose line numbers in GL debug output
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

namespace polyscope {
namespace render {

// == A few enums that contorl behavior

// The drawing modes available
enum class DrawMode {
  Points = 0,
  LinesAdjacency,
  Triangles,
  TrianglesAdjacency,
  Patches,
  IndexedTriangles,
  Lines,
  IndexedLines,
  IndexedLineStrip,
  IndexedLinesAdjacency,
  IndexedLineStripAdjacency
};

enum class FilterMode { Nearest = 0, Linear };
enum class TextureFormat { RGB8 = 0, RGBA8, RGBA32F, RGB32F, R32F };
enum class RenderBufferType { Color, ColorAlpha, Depth, Float4 };

enum class DataType { Vector2Float, Vector3Float, Vector4Float, Matrix44Float, Float, Int, UInt, Index };

// All of these are templated on the backend B. The backend should specialize the low-level functions

class TextureBuffer {
public:
  // abstract class: use the factory methods from the Engine class
  TextureBuffer(int dim_, TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_ = -1);

  virtual ~TextureBuffer();

  // Resize the underlying buffer (contents are lost)
  virtual void resize(unsigned int newLen);
  virtual void resize(unsigned int newX, unsigned int newY);

  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }
  int getDimension() const { return dim; }

  virtual void setFilterMode(FilterMode newMode);

  // Set texture data
  void fillTextureData1D(std::string name, unsigned char* texData, unsigned int length);
  void fillTextureData2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                         bool withAlpha = true, bool useMipMap = false, bool repeat = false);

protected:
  int dim;
  TextureFormat format;

  unsigned int sizeX, sizeY;
};

class RenderBuffer {
public:
  // abstract class: use the factory methods from the Engine class
  RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_);
  virtual ~RenderBuffer(){};

  RenderBufferType getType() const { return type; }
  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }

protected:
  RenderBufferType type;
  unsigned int sizeX, sizeY;
};


class FrameBuffer {

public:
  // abstract class: use the factory methods from the Engine class
  FrameBuffer();
  virtual ~FrameBuffer(){};

  // Bind to this framebuffer so subsequent draw calls will go to it
  // If return is false, binding failed and the framebuffer should not be used.
  virtual bool bindForRendering() = 0;

  // Clear to redraw
  virtual void clear() = 0;
  glm::vec3 clearColor{1.0, 1.0, 1.0};
  float clearAlpha = 1.0;

  // Bind to textures/renderbuffers for output
  // TODO probably don't want these in general
  virtual void bindToColorRenderBuffer(RenderBuffer* renderBuffer) = 0;
  virtual void bindToDepthRenderBuffer(RenderBuffer* renderBuffer) = 0;
  virtual void bindToColorTextureBuffer(TextureBuffer* textureBuffer) = 0;
  virtual void bindToDepthTextureBuffer(TextureBuffer* textureBuffer) = 0;

  // Specify the viewport coordinates and clearcolor
  virtual void setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY);

  // Resizes textures and renderbuffers if different from current size.
  virtual void resizeBuffers(unsigned int newXSize, unsigned int newYSize) = 0;

  // Getters
  // RenderBuffer* getColorRenderBuffer() const { return colorRenderBuffer; }
  // RenderBuffer* getDepthRenderBuffer() const { return depthRenderBuffer; }
  // TextureBuffer* getColorTextureBuffer() const { return colorTextureBuffer; }
  // TextureBuffer* getDepthTextureBuffer() const { return depthTextureBuffer; }

  // Manage buffers
  std::shared_ptr<RenderBuffer> getRenderBuffer(std::string bufferName);
  std::shared_ptr<TextureBuffer> getTextureBuffer(std::string bufferName);

  // Query pixel
  virtual std::array<float, 4> readFloat4(int xPos, int yPos) = 0;

protected:
  // Viewport
  bool viewportSet = false;
  int viewportX, viewportY;
  unsigned int viewportSizeX, viewportSizeY;

  // Buffers
  std::vector<std::pair<std::string, std::shared_ptr<RenderBuffer>>> renderBuffers;
  std::vector<std::pair<std::string, std::shared_ptr<TextureBuffer>>> textureBuffers;
};

// == Shaders

struct ShaderSpecUniform {
  const std::string name;
  const DataType type;
};
struct ShaderSpecAttribute {
  ShaderSpecAttribute(std::string name_, DataType type_) : name(name_), type(type_), arrayCount(1) {}
  ShaderSpecAttribute(std::string name_, DataType type_, int arrayCount_)
      : name(name_), type(type_), arrayCount(arrayCount_) {}
  const std::string name;
  const DataType type;
  const int arrayCount; // number of times this element is repeated in an array
};
struct ShaderSpecTexture {
  const std::string name;
  const int dim;
};


// Types which represents shaders and the values they require
enum class ShaderStageType { Vertex, Tessellation, Evaluation, Geometry, /* Compute,*/ Fragment };
struct ShaderStageSpecification {
  const ShaderStageType stage;
  const std::vector<ShaderSpecUniform> uniforms;
  const std::vector<ShaderSpecAttribute> attributes;
  const std::vector<ShaderSpecTexture> textures;
  const std::string outputLoc;
  const std::string src;
};


// Encapsulate a shader program
class ShaderProgram {

public:
  ShaderProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm, unsigned int nPatchVertices = 0);
  virtual ~ShaderProgram(){};


  // === Store data
  // If update is set to "true", data is updated rather than allocated (must be allocated first)

  // Uniforms
  virtual bool hasUniform(std::string name) = 0;
  virtual void setUniform(std::string name, int val) = 0;
  virtual void setUniform(std::string name, unsigned int val) = 0;
  virtual void setUniform(std::string name, float val) = 0;
  virtual void setUniform(std::string name, double val) = 0; // WARNING casts down to float
  virtual void setUniform(std::string name, float* val) = 0;
  virtual void setUniform(std::string name, glm::vec2 val) = 0;
  virtual void setUniform(std::string name, glm::vec3 val) = 0;
  virtual void setUniform(std::string name, glm::vec4 val) = 0;
  virtual void setUniform(std::string name, std::array<float, 3> val) = 0;
  virtual void setUniform(std::string name, float x, float y, float z, float w) = 0;

  // = Attributes
  // clang-format off
  virtual bool hasAttribute(std::string name) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec2>& data, bool update = false, int offset = 0, int size = -1) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec3>& data, bool update = false, int offset = 0, int size = -1) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec4>& data, bool update = false, int offset = 0, int size = -1) = 0;
  virtual void setAttribute(std::string name, const std::vector<double>& data, bool update = false, int offset = 0, int size = -1) = 0;
  virtual void setAttribute(std::string name, const std::vector<int>& data, bool update = false, int offset = 0, int size = -1) = 0;
  virtual void setAttribute(std::string name, const std::vector<uint32_t>& data, bool update = false, int offset = 0, int size = -1) = 0;
  // clang-format on

  // Convenience method to set an array-valued attrbute, such as 'in vec3 vertexVal[3]'. Applies interleaving then
  // forwards to the usual setAttribute
  template <typename T, unsigned int C>
  void setAttribute(std::string name, const std::vector<std::array<T, C>>& data, bool update = false, int offset = 0,
                    int size = -1);


  // Textures
  virtual void setTexture1D(std::string name, unsigned char* texData, unsigned int length) = 0;
  virtual void setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                            bool withAlpha = true, bool useMipMap = false, bool repeat = false) = 0;
  // virtual void setTextureFromColormap(std::string name, const ValueColorMap& colormap, bool allowUpdate = false) = 0;
  virtual void setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer) = 0;


  // Indices
  virtual void setIndex(std::vector<std::array<unsigned int, 3>>& indices) = 0;
  virtual void setIndex(std::vector<unsigned int>& indices) = 0;
  virtual void setPrimitiveRestartIndex(unsigned int restartIndex) = 0;

  // Call once to initialize GLSL code used by multiple shaders
  static void initCommonShaders(); // TODO

  // Draw!
  virtual void draw() = 0;

  virtual void validateData() = 0;

protected:
  // What mode does this program draw in?
  DrawMode drawMode;

  // How much data is there to draw
  unsigned int drawDataLength;

  // Does this program use indexed drawing?
  bool useIndex = false;
  long int indexSize = -1;
  bool usePrimitiveRestart = false;
  bool primitiveRestartIndexSet = false;
  unsigned int restartIndex = -1;

  // Tessellation parameters
  unsigned int nPatchVertices;
};


class Engine {

public:
  // Options

  // High-level control
  virtual void checkError(bool fatal = false) = 0;

  virtual void clearDisplay() = 0;
  virtual void bindDisplay() = 0;
  
  virtual void clearGBuffer() = 0;
  virtual void computeLighting() = 0;
  virtual void toDisplay() = 0;

  // Small options
  virtual bool bindGBuffer() = 0; // TODO I'm not sure any of these should actually be virtual
  virtual void resizeGBuffer(int width, int height) = 0;
  virtual void setGBufferViewport(int xStart, int yStart, int sizeX, int sizeY) = 0;
  void setBackgroundColor(glm::vec3 newColor);
  void setBackgroundAlpha(float newAlpha);

  // === Factory methods

  // create textures
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               unsigned char* data) = 0; // 1d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               float* data) = 0; // 1d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_,
                                                               unsigned char* data = nullptr) = 0; // 2d

  // create render buffers
  virtual std::shared_ptr<RenderBuffer> generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                             unsigned int sizeY_) = 0;

  // create frame buffers
  virtual std::shared_ptr<FrameBuffer> generateFrameBuffer() = 0;

  // create shader programs
  virtual std::shared_ptr<ShaderProgram> generateShaderProgram(const std::vector<ShaderStageSpecification>& stages,
                                                               DrawMode dm, unsigned int nPatchVertices = 0) = 0;

  // === All of the frame buffers used in the rendering pipeline
  std::unique_ptr<FrameBuffer> GBuffer;

private:
  // TODO Manage a cache of compiled shaders?
};


// Implementation of template functions
template <typename T, unsigned int C>
inline void ShaderProgram::setAttribute(std::string name, const std::vector<std::array<T, C>>& data, bool update,
                                        int offset, int size) {

  // Unpack and forward
  std::vector<T> entryData;
  entryData.reserve(C * data.size());
  for (auto& x : data) {
    for (size_t i = 0; i < C; i++) {
      entryData.push_back(x[i]);
    }
  }
  setAttribute(name, entryData, update, offset, size);
}

// === Public API
// Callers should basically only interact via these methods and variables

// Call once to initialize
void initializeRenderEngine();

// The global render engine
// Gets initialized by initializeRenderEngine() in polyscope::init();
extern Engine* engine;

} // namespace render
} // namespace polyscope

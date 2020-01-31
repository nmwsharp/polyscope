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

template <typename B>
class TextureBuffer {
public:
  // create a 1D texture from data
  TextureBuffer(TextureFormat format, unsigned int size1D, unsigned char* data);
  TextureBuffer(TextureFormat format, unsigned int size1D, float* data);

  // create a 2D texture from data
  TextureBuffer(TextureFormat format, unsigned int sizeX_, unsigned int sizeY_, unsigned char* data = nullptr);

  ~TextureBuffer();

  void setFilterMode(FilterMode newMode);
  void bind();

  // Resize the underlying buffer (contents are lost)
  void resize(unsigned int newLen);
  void resize(unsigned int newX, unsigned int newY);

  typename B::TextureBufferHandle getHandle() const { return handle; }
  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }
  int getDimension() const { return dim; }

private:
  typename B::TextureBufferHandle handle;
  TextureFormat format;
  unsigned int sizeX, sizeY;
  int dim;
};

template <typename B>
class RenderBuffer {
public:
  RenderBuffer(RenderBufferType type, unsigned int sizeX_, unsigned int sizeY_);
  ~RenderBuffer();

  void bind();
  typename B::RenderBufferHandle getHandle() const { return handle; }
  RenderBufferType getType() const { return type; }
  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }

private:
  typename B::RenderBufferHandle handle;
  RenderBufferType type;
  unsigned int sizeX, sizeY;
};


template <typename B>
class FrameBuffer {

public:
  FrameBuffer();
  ~FrameBuffer();

  // Bind to this framebuffer so subsequent draw calls will go to it
  // If return is false, binding failed and the framebuffer should not be used.
  bool bindForRendering();

  // Clear to redraw
  void clear();

  // Bind to textures/renderbuffers for output
  void bindToColorRenderbuffer(RenderBuffer<B>* renderBuffer);
  void bindToDepthRenderbuffer(RenderBuffer<B>* renderBuffer);
  void bindToColorTexturebuffer(TextureBuffer<B>* textureBuffer);
  void bindToDepthTexturebuffer(TextureBuffer<B>* textureBuffer);

  // Specify the viewport coordinates and clearcolor
  void setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY);
  glm::vec3 clearColor{0.0, 0.0, 0.0};
  float clearAlpha = 1.0;

  // Resizes textures and renderbuffers if different from current size.
  void resizeBuffers(unsigned int newXSize, unsigned int newYSize);

  // Getters
  typename B::FrameBufferHandle getHandle() const { return handle; }
  RenderBuffer<B>* getColorRenderBuffer() const { return colorRenderBuffer; }
  RenderBuffer<B>* getDepthRenderBuffer() const { return depthRenderBuffer; }
  TextureBuffer<B>* getColorTextureBuffer() const { return colorTextureBuffer; }
  TextureBuffer<B>* getDepthTextureBuffer() const { return depthTextureBuffer; }

  // Query pixel
  std::array<float, 4> readFloat4(int xPos, int yPos);

private:
  typename B::FrameBufferHandle handle;

  // Will have a renderbuffer, a texturebuffer, or neither for each of depth and color.
  RenderBuffer<B>* colorRenderBuffer = nullptr;
  TextureBuffer<B>* colorTextureBuffer = nullptr;
  RenderBuffer<B>* depthRenderBuffer = nullptr;
  TextureBuffer<B>* depthTextureBuffer = nullptr;

  // Viewport
  bool viewportSet = false;
  int viewportX, viewportY;
  unsigned int viewportSizeX, viewportSizeY;
};

// == Shaders

struct ShaderUniform {
  const std::string name;
  const DataType type;
};
struct ShaderAttribute {
  ShaderAttribute(std::string name_, DataType type_) : name(name_), type(type_), arrayCount(1) {}
  ShaderAttribute(std::string name_, DataType type_, int arrayCount_)
      : name(name_), type(type_), arrayCount(arrayCount_) {}
  const std::string name;
  const DataType type;
  const int arrayCount; // number of times this element is repeated in an array
};
struct ShaderTexture {
  const std::string name;
  const int dim;
};


// Types which represents shaders and the values they require
template <typename B>
struct VertexShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
template <typename B>
struct TessellationShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
template <typename B>
struct EvaluationShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
template <typename B>
struct GeometryShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::string src;
};
template <typename B>
struct FragmentShader {
  const std::vector<ShaderUniform> uniforms;
  const std::vector<ShaderAttribute> attributes;
  const std::vector<ShaderTexture> textures;
  const std::string outputLoc;
  const std::string src;
};


// Encapsulate a shader program
template <typename B>
class ShaderProgram {

public:
  // Constructors
  ShaderProgram(const VertexShader<B>* vShader, const FragmentShader<B>* fShader, DrawMode dm);
  ShaderProgram(const VertexShader<B>* vShader, const GeometryShader<B>* gShader, const FragmentShader<B>* fShader,
                DrawMode dm);
  ShaderProgram(const VertexShader<B>* vShader, const TessellationShader<B>* tShader, const FragmentShader<B>* fShader,
                DrawMode dm, int nPatchVertices);
  ShaderProgram(const VertexShader<B>* vShader, const EvaluationShader<B>* eShader, const FragmentShader<B>* fShader,
                DrawMode dm, int nPatchVertices);
  ShaderProgram(const VertexShader<B>* vShader, const TessellationShader<B>* tShader,
                const EvaluationShader<B>* eShader, const FragmentShader<B>* fShader, DrawMode dm, int nPatchVertices);
  ShaderProgram(const VertexShader<B>* vShader, const TessellationShader<B>* tShader,
                const EvaluationShader<B>* eShader, const GeometryShader<B>* gShader, const FragmentShader<B>* fShader,
                DrawMode dm, int nPatchVertices);

  // Destructor (free gl buffers)
  ~ShaderProgram();


  // === Store data
  // If update is set to "true", data is updated rather than allocated (must be allocated first)

  // Uniforms
  bool hasUniform(std::string name);
  void setUniform(std::string name, int val);
  void setUniform(std::string name, unsigned int val);
  void setUniform(std::string name, float val);
  void setUniform(std::string name, double val); // WARNING casts down to float
  void setUniform(std::string name, float* val);
  void setUniform(std::string name, glm::vec2 val);
  void setUniform(std::string name, glm::vec3 val);
  void setUniform(std::string name, glm::vec4 val);
  void setUniform(std::string name, std::array<float, 3> val);
  void setUniform(std::string name, float x, float y, float z, float w);

  // = Attributes
  bool hasAttribute(std::string name);
  void setAttribute(std::string name, const std::vector<glm::vec2>& data, bool update = false, int offset = 0,
                    int size = -1);
  void setAttribute(std::string name, const std::vector<glm::vec3>& data, bool update = false, int offset = 0,
                    int size = -1);
  void setAttribute(std::string name, const std::vector<glm::vec4>& data, bool update = false, int offset = 0,
                    int size = -1);
  void setAttribute(std::string name, const std::vector<double>& data, bool update = false, int offset = 0,
                    int size = -1);
  void setAttribute(std::string name, const std::vector<int>& data, bool update = false, int offset = 0, int size = -1);
  void setAttribute(std::string name, const std::vector<uint32_t>& data, bool update = false, int offset = 0,
                    int size = -1);

  // Convenience method to set an array-valued attrbute, such as 'in vec3 vertexVal[3]'. Applies interleaving then
  // forwards to the usual setAttribute
  template <typename T, unsigned int C>
  void setAttribute(std::string name, const std::vector<std::array<T, C>>& data, bool update = false, int offset = 0,
                       int size = -1);


  // Textures
  void setTexture1D(std::string name, unsigned char* texData, unsigned int length);
  void setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                    bool withAlpha = true, bool useMipMap = false, bool repeat = false);
  // void setTextureFromColormap(std::string name, const ValueColorMap& colormap, bool allowUpdate = false); TODO
  void setTextureFromBuffer(std::string name, TextureBuffer<B>* textureBuffer);


  // Indices
  void setIndex(std::vector<std::array<unsigned int, 3>>& indices);
  void setIndex(std::vector<unsigned int>& indices);
  void setPrimitiveRestartIndex(unsigned int restartIndex);

  // Call once to initialize GLSL code used by multiple shaders
  static void initCommonShaders();

  // Draw!
  void draw();

private:
  // Classes to keep track of attributes and uniforms
  struct Uniform {
    std::string name;
    DataType type;
    typename B::UniformLocation location;
    bool isSet; // has a value been assigned to this uniform?
  };
  struct Attribute {
    std::string name;
    DataType type;
    typename B::AttributeLocation location;
    typename B::VertexBufferHandle VBOLoc;
    long int dataSize; // the size of the data currently stored in this attribute (-1 if nothing)
    int arrayCount;
  };
  struct Texture {
    std::string name;
    int dim;
    typename B::TextureLocation location;
    TextureBuffer<B>* textureBuffer;
    unsigned int index;
    bool isSet;
    bool managedByProgram; // should the program delete the texture when its done?
  };


  // The shader objects to use, which generally come from shaders.h
  const VertexShader<B>* vertShader = nullptr;
  const TessellationShader<B>* tessShader = nullptr;
  const EvaluationShader<B>* evalShader = nullptr;
  const GeometryShader<B>* geomShader = nullptr;
  const FragmentShader<B>* fragShader = nullptr;


  // Lists of attributes and uniforms that need to be set
  std::vector<Uniform> uniforms;
  std::vector<Attribute> attributes;
  std::vector<Texture> textures;

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

  // Setup routines
  void compileGLProgram();
  void setDataLocations();
  void createBuffers();
  void addUniqueAttribute(ShaderAttribute attribute);
  void deleteAttributeBuffer(Attribute attribute);
  void addUniqueUniform(ShaderUniform uniform);
  void addUniqueTexture(ShaderTexture texture);
  void freeTexture(Texture t);

  // Drawing related
  void validateData();
  void activateTextures();

  // GL pointers for various useful things
  typename B::ProgramHandle programHandle = 0;
  typename B::ShaderHandle vertShaderHandle = 0; // vertex shader
  typename B::ShaderHandle tessShaderHandle = 0; // tessellation control shader
  typename B::ShaderHandle evalShaderHandle = 0; // tesselation evaluation shader
  typename B::ShaderHandle geomShaderHandle = 0; // geometry shader
  typename B::ShaderHandle fragShaderHandle = 0; // fragment shader
  typename B::AttributeHandle vaoHandle;         // TODO rename
  typename B::AttributeHandle indexVBO;          // TODO rename

  //static typename B::ShaderHandle commonShaderHandle; // functions accessible to all shaders TODO move to engine
};


template <typename B>
class Engine {

public:
  // Options


  // High-level control
  void initialize();
  void clearGBuffer();
  void computeLighting();
  void toDisplay();


  // === All of the frame buffers used in the rendering pipeline
  std::unique_ptr<FrameBuffer<B>> GBuffer;

private:
  // Manage a cache of compiled shaders
};


// === Utility functions
template <typename B>
void printShaderInfoLog(typename B::ShaderHandle shaderHandle);

template <typename B>
void printProgramInfoLog(typename B::ProgramHandle handle);

template <typename B>
void checkError(bool fatal = false);


// Implementation of template functions
template <typename B>
template <typename T, unsigned int C>
inline void ShaderProgram<B>::setAttribute(std::string name, const std::vector<std::array<T, C>>& data, bool update,
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

} // namespace render
} // namespace polyscope

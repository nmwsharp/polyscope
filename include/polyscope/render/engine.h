// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "polyscope/render/color_maps.h"
#include "polyscope/render/ground_plane.h"
#include "polyscope/render/materials.h"
#include "polyscope/types.h"
#include "polyscope/view.h"

#include "imgui.h"

namespace polyscope {

// Forward declarations for ManagedBuffer integration
namespace render {
class ManagedBufferBase;
template <typename T>
class ManagedBuffer;
} // namespace render

// == A few enums that control behavior
// public enums are in the outer namespace to keep the typing burden down

// The drawing modes available
enum class DrawMode {
  Points = 0,
  LinesAdjacency,
  Triangles,
  TrianglesAdjacency,
  IndexedTriangles,
  Lines,
  IndexedLines,
  IndexedLineStrip,
  IndexedLinesAdjacency,
  IndexedLineStripAdjacency,
  TrianglesInstanced,
  TriangleStripInstanced,
};

enum class TextureFormat { RGB8 = 0, RGBA8, RG16F, RGB16F, RGBA16F, RGBA32F, RGB32F, R32F, R16F, DEPTH24 };
enum class RenderBufferType { Color, ColorAlpha, Depth, Float4 };
enum class DepthMode { Less, LEqual, LEqualReadOnly, Greater, Disable, PassReadOnly };
enum class BlendMode { AlphaOver, OverNoWrite, AlphaUnder, Zero, WeightedAdd, Add, Source, Disable };
enum class RenderDataType {
  Vector2Float,
  Vector3Float,
  Vector4Float,
  Matrix44Float,
  Float,
  Int,
  Vector2Int,
  Vector3Int,
  Vector4Int,
  UInt,
  Vector2UInt,
  Vector3UInt,
  Vector4UInt
};

enum class DeviceBufferType { Attribute, Texture1d, Texture2d, Texture3d };
enum class ShaderTextureType { Texture1D, Texture2D, Texture3D, TextureBuffer };

int dimension(const TextureFormat& x);
int sizeInBytes(const TextureFormat& f);
std::string modeName(const TransparencyMode& m);
std::string renderDataTypeName(const RenderDataType& r);
int sizeInBytes(const RenderDataType& r);
int renderDataTypeCountCompatbility(const RenderDataType r1, const RenderDataType r2);
std::string getImageOriginRule(ImageOrigin imageOrigin);
std::string deviceBufferTypeName(const DeviceBufferType& d);
std::string shaderTextureTypeName(const ShaderTextureType& t);

namespace render {

class AttributeBuffer {
public:
  AttributeBuffer(RenderDataType dataType_, int arrayCount);

  virtual ~AttributeBuffer();

  // Upload up to `count` elements from `data` starting at GPU element `offset` (defaults to full `data` at 0).
  virtual void setData(const std::vector<glm::vec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::vec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::vec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<float>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<double>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<int32_t>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<uint32_t>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;

  // Array-valued attributes
  // (adding these lazily as we need them)
  // (sadly we cannot template the virtual function)
  virtual void setData(const std::vector<std::array<glm::vec3, 2>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 3>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 4>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;

  // Allocate GPU memory for exactly n elements without uploading any data (no-op if already set to n).
  virtual void setBufferSize(size_t n) = 0;

  virtual uint32_t getNativeBufferID() = 0; // used to interop with external things, e.g. ImGui

  // == Getters
  RenderDataType getType() const { return dataType; }
  int getArrayCount() const { return arrayCount; }
  uint64_t getBufferSize() const { return bufferSize; }
  uint64_t getUniqueID() const { return uniqueID; }
  bool isSet() const { return setFlag; }

  // get data at a single index from the buffer
  virtual float getData_float(size_t ind) = 0;
  virtual double getData_double(size_t ind) = 0;
  virtual glm::vec2 getData_vec2(size_t ind) = 0;
  virtual glm::vec3 getData_vec3(size_t ind) = 0;
  virtual glm::vec4 getData_vec4(size_t ind) = 0;
  virtual int getData_int(size_t ind) = 0;
  virtual glm::ivec2 getData_ivec2(size_t ind) = 0;
  virtual glm::ivec3 getData_ivec3(size_t ind) = 0;
  virtual glm::ivec4 getData_ivec4(size_t ind) = 0;
  virtual uint32_t getData_uint32(size_t ind) = 0;
  virtual glm::uvec2 getData_uvec2(size_t ind) = 0;
  virtual glm::uvec3 getData_uvec3(size_t ind) = 0;
  virtual glm::uvec4 getData_uvec4(size_t ind) = 0;

  // get data at a range of indices from the buffer
  virtual std::vector<float> getDataRange_float(size_t ind, size_t count) = 0;
  virtual std::vector<double> getDataRange_double(size_t ind, size_t count) = 0;
  virtual std::vector<glm::vec2> getDataRange_vec2(size_t ind, size_t count) = 0;
  virtual std::vector<glm::vec3> getDataRange_vec3(size_t ind, size_t count) = 0;
  virtual std::vector<glm::vec4> getDataRange_vec4(size_t ind, size_t count) = 0;
  virtual std::vector<int> getDataRange_int(size_t ind, size_t count) = 0;
  virtual std::vector<glm::ivec2> getDataRange_ivec2(size_t ind, size_t count) = 0;
  virtual std::vector<glm::ivec3> getDataRange_ivec3(size_t ind, size_t count) = 0;
  virtual std::vector<glm::ivec4> getDataRange_ivec4(size_t ind, size_t count) = 0;
  virtual std::vector<uint32_t> getDataRange_uint32(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec2> getDataRange_uvec2(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec3> getDataRange_uvec3(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec4> getDataRange_uvec4(size_t ind, size_t count) = 0;

protected:
  RenderDataType dataType;
  int arrayCount;
  bool setFlag = false;
  uint64_t bufferSize = 0; // size of the allocated GPU buffer in elements of this buffer's type
  uint64_t uniqueID;
};

class TextureBuffer {
public:
  // abstract class: use the factory methods from the Engine class
  TextureBuffer(int dim_, TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_ = -1,
                unsigned int sizeZ_ = -1);

  virtual ~TextureBuffer();

  // Resize the underlying buffer (contents are lost)
  virtual void resize(unsigned int newLen);
  virtual void resize(unsigned int newX, unsigned int newY);
  virtual void resize(unsigned int newX, unsigned int newY, unsigned int newZ);

  // Fill with data (uploads up to `count` elements at `offset`; non-zero offset is only valid for 1D textures)
  // NOTE: some of these are not implemented yet
  virtual void setData(const std::vector<glm::vec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::vec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::vec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<float>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<double>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<int32_t>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::ivec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<uint32_t>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec2>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec3>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<glm::uvec4>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;

  // Array-valued
  // NOTE: some of these are not implemented yet
  // (adding these lazily as we need them)
  // (sadly we cannot template the virtual function)
  virtual void setData(const std::vector<std::array<glm::vec3, 2>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 3>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 4>>& data, size_t offset = 0,
                       size_t count = std::numeric_limits<size_t>::max()) = 0;

  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }
  unsigned int getSizeZ() const { return sizeZ; }
  int getDimension() const { return dim; }
  unsigned int getTotalSize() const; // product of dimensions
  int64_t getSizeInBytes() const { return static_cast<int64_t>(getTotalSize()) * sizeInBytes(format); }
  uint64_t getUniqueID() const { return uniqueID; }
  TextureFormat getFormat() const { return format; }

  virtual void setFilterMode(FilterMode newMode);

  // Get texture data CPU-side
  // (call the version which matches the dimension of the texture datatype, otherwise you will get an error. remember
  // that the texture datatype is a distinct concepts from its spatial dimension stored in dim)
  virtual std::vector<float> getDataScalar() = 0;
  virtual std::vector<glm::vec2> getDataVector2() = 0;
  virtual std::vector<glm::vec3> getDataVector3() = 0;

  // Set texture data
  // void fillTextureData1D(std::string name, unsigned char* texData, unsigned int length);
  // void fillTextureData2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
  // bool withAlpha = true, bool useMipMap = false, bool repeat = false);

  // used to interop with external things, e.g. ImGui
  virtual void* getNativeHandle() = 0;
  virtual uint32_t getNativeBufferID() = 0;

protected:
  int dim;
  TextureFormat format;
  unsigned int sizeX, sizeY, sizeZ;
  uint64_t uniqueID;
};

class RenderBuffer {
public:
  // abstract class: use the factory methods from the Engine class
  RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_);
  virtual ~RenderBuffer(){};

  virtual void resize(unsigned int newX, unsigned int newY);

  RenderBufferType getType() const { return type; }
  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }
  uint64_t getUniqueID() const { return uniqueID; }

protected:
  RenderBufferType type;
  unsigned int sizeX, sizeY;
  uint64_t uniqueID;
};


class FrameBuffer {

public:
  // abstract class: use the factory methods from the Engine class
  FrameBuffer();
  virtual ~FrameBuffer(){};

  virtual void bind() = 0;
  // Bind to this framebuffer so subsequent draw calls will go to it
  // If return is false, binding failed and the framebuffer should not be used.
  virtual bool bindForRendering() = 0;

  // Clear to redraw
  virtual void clear() = 0;
  glm::vec3 clearColor{1.0, 1.0, 1.0};
  float clearAlpha = 0.0;
  float clearDepth = 1.0;

  // Bind to textures/renderbuffers for output
  // note: currently no way to remove buffers
  virtual void addColorBuffer(std::shared_ptr<RenderBuffer> renderBuffer) = 0;
  virtual void addColorBuffer(std::shared_ptr<TextureBuffer> textureBuffer) = 0;
  virtual void addDepthBuffer(std::shared_ptr<RenderBuffer> renderBuffer) = 0;
  virtual void addDepthBuffer(std::shared_ptr<TextureBuffer> textureBuffer) = 0;

  virtual void setDrawBuffers() = 0;

  // Specify the viewport coordinates
  virtual void setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY);

  // Resizes textures and renderbuffers if different from current size.
  // We will always maintain that all bound color and depth buffers have the same size as the
  // framebuffer size.
  virtual void resize(unsigned int newXSize, unsigned int newYSize);
  unsigned int getSizeX() const { return sizeX; }
  unsigned int getSizeY() const { return sizeY; }
  void verifyBufferSizes();

  // Query pixel
  virtual std::array<float, 4> readFloat4(int xPos, int yPos) = 0;
  virtual float readDepth(int xPos, int yPos) = 0;
  virtual void blitTo(FrameBuffer* other, int targetPosX = 0, int targetPosY = 0) = 0;
  virtual std::vector<unsigned char> readBuffer() = 0;

  virtual uint32_t getNativeBufferID() = 0;
  uint64_t getUniqueID() const { return uniqueID; }

protected:
  unsigned int sizeX, sizeY;
  uint64_t uniqueID;

  // Viewport
  bool viewportSet = false;
  int viewportX, viewportY;
  unsigned int viewportSizeX, viewportSizeY;

  // Buffers
  int nColorBuffers = 0;
  std::vector<std::shared_ptr<RenderBuffer>> renderBuffersColor, renderBuffersDepth;
  std::vector<std::shared_ptr<TextureBuffer>> textureBuffersColor, textureBuffersDepth;
};

// == Shaders

struct ShaderSpecUniform {
  const std::string name;
  const RenderDataType type;
};
struct ShaderSpecAttribute {
  ShaderSpecAttribute(std::string name_, RenderDataType type_) : name(name_), type(type_), arrayCount(1) {}
  ShaderSpecAttribute(std::string name_, RenderDataType type_, int arrayCount_)
      : name(name_), type(type_), arrayCount(arrayCount_) {}
  const std::string name;
  const RenderDataType type;
  const int arrayCount; // number of times this element is repeated in an array
};
struct ShaderSpecTexture {
  const std::string name;
  const ShaderTextureType type;
};


// Types which represents shaders and the values they require
enum class ShaderStageType { Vertex, Geometry, /* Compute,*/ Fragment };
struct ShaderStageSpecification {
  const ShaderStageType stage;
  const std::vector<ShaderSpecUniform> uniforms;
  const std::vector<ShaderSpecAttribute> attributes;
  const std::vector<ShaderSpecTexture> textures;
  const std::string src;
};

// A simple interface for replacement rules to customize shaders
// The "replacements" are key-value pairs will be used to modify the program source. Each key corresponds to a tag in
// the program source, which will be replaced by the string value (if many such replacements exist, the values are
// concatenated together).
// The uniforms/attributes/textures are unioned to the respective lists for the program.
class ShaderReplacementRule {
public:
  ShaderReplacementRule();
  ShaderReplacementRule(std::string ruleName_, std::vector<std::pair<std::string, std::string>> replacements_);
  ShaderReplacementRule(std::string ruleName_, std::vector<std::pair<std::string, std::string>> replacements_,
                        std::vector<ShaderSpecUniform> uniforms_, std::vector<ShaderSpecAttribute> attributes_,
                        std::vector<ShaderSpecTexture> textures_);

  std::string ruleName;
  std::vector<std::pair<std::string, std::string>> replacements;
  std::vector<ShaderSpecUniform> uniforms;
  std::vector<ShaderSpecAttribute> attributes;
  std::vector<ShaderSpecTexture> textures;
};
enum class ShaderReplacementDefaults {
  SceneObject,        // an object in the scene, which gets lit via matcap (etc)
  SceneObjectNoSlice, // like SceneObject, but omits slice-related rules
  Pick,               // rendering to a pick buffer
  Process,            // postprocessing effects, etc
  None                // no defaults applied
};

// Encapsulate a shader program
class ShaderProgram {

public:
  ShaderProgram(DrawMode dm);
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
  virtual void setUniform(std::string name, glm::ivec2 val) = 0;
  virtual void setUniform(std::string name, glm::ivec3 val) = 0;
  virtual void setUniform(std::string name, glm::ivec4 val) = 0;
  virtual void setUniform(std::string name, glm::uvec2 val) = 0;
  virtual void setUniform(std::string name, glm::uvec3 val) = 0;
  virtual void setUniform(std::string name, glm::uvec4 val) = 0;

  // = Attributes
  // clang-format off
  virtual bool hasAttribute(std::string name) = 0;
  virtual bool attributeIsSet(std::string name) = 0;
  virtual std::shared_ptr<AttributeBuffer> getAttributeBuffer(std::string name) = 0;
  virtual void setAttribute(std::string name, std::shared_ptr<AttributeBuffer> externalBuffer, ManagedBufferBase* source = nullptr) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec2>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec3>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec4>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<float>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<double>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<int32_t>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<uint32_t>& data) = 0;
  
  virtual void setAttribute(std::string name, const std::vector<std::array<glm::vec3, 2>>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<std::array<glm::vec3, 3>>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<std::array<glm::vec3, 4>>& data) = 0;
  // clang-format on


  // Textures
  virtual bool hasTexture(std::string name) = 0;
  virtual bool textureIsSet(std::string name) = 0;
  virtual void setTexture1D(std::string name, unsigned char* texData, unsigned int length) = 0;
  virtual void setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                            bool withAlpha = true, bool useMipMap = false, bool repeat = false) = 0;
  virtual void setTextureFromColormap(std::string name, const std::string& colorMap, bool allowUpdate = false) = 0;
  // TODO make this one take a shared pointer and have the same semantics as the attribute version
  virtual void setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer,
                                    ManagedBufferBase* source = nullptr) = 0;
  // Bind an AttributeBuffer (VBO) as a buffer texture (samplerBuffer / GL_TEXTURE_BUFFER,
  // ShaderTextureType::TextureBuffer). Note: the buffer is always exposed to shaders as a flattened 1-component scalar
  // stream (GL_R32F / GL_R32I / GL_R32UI), even for vector/matrix types (e.g., fetch a vec3 at index i via texelFetch
  // at 3*i + {0, 1, 2}). This is done because OpenGL 3.3 core does not support 3-component buffer texture formats
  // (GL_RGB32F requires GL 4.0).
  virtual void setTextureFromAttributeBuffer(std::string name, std::shared_ptr<AttributeBuffer> externalBuffer,
                                             ManagedBufferBase* source = nullptr) = 0;


  // Convenience overloads for ManagedBuffer — set the buffer AND record the source for lazy sync.
  // Defined in managed_buffer.h after ManagedBuffer<T> is fully declared.
  template <typename T>
  void setAttribute(std::string name, ManagedBuffer<T>& buf);
  // Binds a texture-backed ManagedBuffer (DeviceBufferType::Texture1d/2d/3d) to a sampler1D/2D/3D.
  template <typename T>
  void setTextureFromBuffer(std::string name, ManagedBuffer<T>& buf);
  // Binds an attribute-backed ManagedBuffer (DeviceBufferType::Attribute) as a buffer texture (samplerBuffer /
  // TextureBuffer).
  template <typename T>
  void setTextureFromAttributeBuffer(std::string name, ManagedBuffer<T>& buf);
  template <typename T>
  void setIndex(ManagedBuffer<T>& buf);

  // Indices
  virtual void setIndex(std::shared_ptr<AttributeBuffer> externalBuffer) = 0;
  virtual void setPrimitiveRestartIndex(unsigned int restartIndex) = 0;

  // Set the number of primitives to draw in the draw call. Must be called before drawing.
  // For non-indexed modes: number of vertices (e.g. points, line endpoints, triangle corners).
  // For indexed modes: number of index-buffer entries (e.g. uvec3 faces for IndexedTriangles),
  //   the engine multiplies by indexSizeMult internally to get the total index count passed to GL.
  // For instanced modes: number of vertices per instance (instanceCount sets the instance count separately).
  void setDrawCount(uint32_t count) { drawCount = count; }

  // Indices
  virtual void setInstanceCount(uint32_t instanceCount) = 0;

  // Call once to initialize GLSL code used by multiple shaders
  static void initCommonShaders(); // TODO

  // Draw!
  virtual void draw() = 0;

  virtual void validateData() = 0;

  uint64_t getUniqueID() const { return uniqueID; }

protected:
  // What mode does this program draw in?
  DrawMode drawMode;


  // Indexed drawing
  bool useIndex = false;
  uint32_t indexSizeMult = -1;
  bool usePrimitiveRestart = false;
  bool primitiveRestartIndexSet = false;
  unsigned int restartIndex = -1;

  uint64_t uniqueID;

  std::shared_ptr<AttributeBuffer> indexBuffer;
  ManagedBufferBase* indexSourceManagedBuffer = nullptr;

  uint32_t drawCount = INVALID_IND_32;

  // instancing
  uint32_t instanceCount = INVALID_IND_32;
};


struct SceneDrawCallbacks {
  std::function<void()> drawStructures;
  std::function<void()> drawStructuresDelayed;
  std::function<void()> drawSlicePlanes;
};

struct PickBufferSample {
  std::array<float, 4> color{0.f, 0.f, 0.f, 0.f};
  float clipDepth = 1.0f;
  bool valid = false;
};

class Engine {

public:
  Engine();
  virtual ~Engine();

  // High-level control
  virtual void shutdown() {};
  virtual void checkError(bool fatal = false) = 0;
  void buildEngineGui();

  // 'headless' means there is no physical display to actually render to, e.g. when running on a remote server
  virtual bool isHeadless() { return false; }

  virtual void clearDisplay();
  virtual bool bindDisplay();
  virtual void swapDisplayBuffers() = 0;
  void pushBindFramebufferForRendering(
      FrameBuffer& f); // push the existing rendering framebuffer on to a stack and bind to f for rendering
  void popBindFramebufferForRendering(); // pop the old framebuffer off the stack and bind to it
  virtual std::vector<unsigned char> readDisplayBuffer() = 0;

  void setUseAltDisplayBuffer(bool newVal); // if true, push final render results offscreen to the alt buffer instead
  std::vector<unsigned char> readAltDisplayBuffer();

  virtual void resizeScreenBuffersAndSetViewports(); // applies to all buffers tied to display size
  void renderBackground();                           // respects background setting

  // Manage render state
  virtual void setDepthMode(DepthMode newMode) = 0;
  virtual void setBlendMode(BlendMode newMode) = 0;
  virtual void setColorMask(std::array<bool, 4> mask = {true, true, true, true}) = 0;
  virtual void setBackfaceCull(bool newVal = false) = 0;
  virtual void setViewportAndScissor(int startX, int startY, unsigned int sizeX, unsigned int sizeY);
  virtual void setScissor(int startX, int startY, unsigned int sizeX, unsigned int sizeY);
  virtual void disableScissor();
  bool isScissorEnabled() const;

  void setCurrentViewport(glm::vec4 viewport);
  glm::vec4 getCurrentViewport();
  void setCurrentPixelScaling(float scale);
  float getCurrentPixelScaling();

  // Helpers
  void allocateGlobalBuffersAndPrograms(); // called once during startup

  // Small options
  void setBackgroundColor(glm::vec3 newColor);
  void setBackgroundAlpha(float newAlpha);

  // Manage materials
  void setMaterial(ShaderProgram& program, const std::string& mat);
  std::vector<std::string> addMaterialRules(std::string materialName, std::vector<std::string> initRules);
  void setMaterialUniforms(ShaderProgram& program, const std::string& mat);

  // === Scene rendering pipeline
  void renderScene(const SceneDrawCallbacks& callbacks);
  void renderSceneToDisplay();
  void renderDebugPickToDisplay(const std::function<void()>& drawPickFn);

  // Render pick geometry for the active viewport and read back color + clip-space depth at local buffer coords.
  // Passing xPos == -1 or yPos == -1 renders the pick buffer without reading back a pixel.
  PickBufferSample renderAndReadPick(int xPos, int yPos, const std::function<void()>& drawPickFn);

  void showDebugTexturesInImGui();

  bool bindSceneBuffer();
  TextureBuffer& getFinalSceneColorTexture();
  TextureBuffer* getSceneDepthMinTexture();

  GroundPlane groundPlane;

  // === Windowing and framework things
  virtual void makeContextCurrent() = 0;
  virtual void focusWindow() = 0;
  virtual void showWindow() = 0;
  virtual void hideWindow() = 0;
  virtual void updateWindowSize(bool force = false) = 0;
  virtual void applyWindowSize() = 0;               // forces the current window size to match view::windowWidth/Height
  virtual void setWindowResizable(bool newVal) = 0; // whether the user can manually resize by dragging the window frame
  virtual bool getWindowResizable() = 0;
  virtual std::tuple<int, int> getWindowPos() = 0;
  virtual bool windowRequestsClose() = 0;
  virtual void pollEvents() = 0;
  virtual bool isKeyPressed(char c) = 0; // for lowercase a-z and 0-9 only
  virtual std::string getClipboardText() = 0;
  virtual void setClipboardText(std::string text) = 0;

  // === ImGui

  // NOTE: the imgui backend depends on the window manager (e.g. GLFW), so these must be implemented by the lowest-level
  // concrete engine implementation
  virtual void createNewImGuiContext() = 0;
  virtual void updateImGuiContext(ImGuiContext* oldContext, ImGuiIO* oldIO, ImGuiContext* newContext, ImGuiIO* newIO) {}
  virtual void shutdownImGui() = 0;
  virtual void ImGuiNewFrame() = 0;
  virtual void ImGuiRender() = 0;

  void setImGuiStyle();

  // Display an ImGui window showing a texture
  // WARNING: you must ensure that the texture buffer pointer stays valid until after the ImGui frame is rendered, which
  // is not until the end of a main loop iteration.
  virtual void showTextureInImGuiWindow(std::string windowName, TextureBuffer* buffer);


  // === Factory methods

  // create attribute buffers
  virtual std::shared_ptr<AttributeBuffer> generateAttributeBuffer(RenderDataType dataType_, int arrayCount_ = 1) = 0;


  // create textures
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               const unsigned char* data = nullptr) = 0; // 1d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                               const float* data) = 0; // 1d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_,
                                                               const unsigned char* data = nullptr) = 0; // 2d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_,
                                                               const float* data) = 0; // 2d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_, unsigned int sizeZ_,
                                                               const unsigned char* data = nullptr) = 0; // 3d
  virtual std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_,
                                                               unsigned int sizeY_, unsigned int sizeZ_,
                                                               const float* data) = 0; // 3d

  // create render buffers
  virtual std::shared_ptr<RenderBuffer> generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                             unsigned int sizeY_) = 0;
  // create frame buffers
  virtual std::shared_ptr<FrameBuffer> generateFrameBuffer(unsigned int sizeX_, unsigned int sizeY_) = 0;

  // == create shader programs
  virtual std::shared_ptr<ShaderProgram>
  requestShader(const std::string& programName, const std::vector<std::string>& customRules,
                ShaderReplacementDefaults defaults = ShaderReplacementDefaults::SceneObject) = 0;

  // === Display frame buffers (managed by the backend / engine)
  // The size of these buffers is always kept in sync with the screen size
  std::shared_ptr<FrameBuffer> displayBuffer, displayBufferAlt;
  FrameBuffer& getDisplayBuffer();

  // General-use programs used by the engine
  // TODO some of these seem to be unused; remove them
  std::shared_ptr<ShaderProgram> renderTexturePlain, renderTextureDot3, renderTextureMap3, renderTextureSphereBG;

  // Manage transparency and culling
  void setTransparencyMode(TransparencyMode newMode);
  TransparencyMode getTransparencyMode();
  bool transparencyEnabled();
  void setPeelEnabled(bool newVal); // if false, skips the min-depth peel filter (e.g. during ground plane shadow passes)
  bool getPeelEnabled() const;
  virtual void applyTransparencySettings() = 0;
  void addSlicePlane(std::string uniquePostfix); // TODO move slice planes out of the engine
  void removeSlicePlane(std::string uniquePostfix);
  bool slicePlanesEnabled();                     // true if there is at least one slice plane in the scene
  virtual void setFrontFaceCCW(bool newVal) = 0; // true if CCW triangles are considered front-facing; false otherwise
  bool getFrontFaceCCW();

  // == Options
  BackgroundView background = BackgroundView::None;

  float exposure = 1.1;
  float whiteLevel = 1.0;
  float gamma = 2.2;
  void setTonemapUniforms(ShaderProgram& p);

  void setSSAAFactor(int newVal);
  int getSSAAFactor();


  // == Cached data

  // Materials
  std::vector<std::unique_ptr<Material>> materials;
  Material& getMaterial(const std::string& name);
  void loadBlendableMaterial(std::string matName, std::array<std::string, 4> filenames);
  void loadBlendableMaterial(std::string matName, std::string filenameBase, std::string filenameExt);
  void loadStaticMaterial(std::string matName, std::string filename);

  // Color maps
  std::vector<std::unique_ptr<ValueColorMap>> colorMaps;
  const ValueColorMap& getColorMap(const std::string& name);
  void loadColorMap(std::string cmapName, std::string filename);
  std::shared_ptr<TextureBuffer> getColorMapTexture2d(const std::string& cmapName);

  // Helpers
  std::vector<glm::vec3> screenTrianglesCoords(); // two triangles which cover the screen
  std::vector<glm::vec4> distantCubeCoords();     // cube with vertices at infinity

  uint64_t getNextUniqueID();

  // Internal windowing and engine details
  FrameBuffer* currRenderFramebuffer = nullptr;

  // ImGui and Fonts
  virtual void configureImGui() {}; // generates font things
  ImFontAtlas* getSharedFontAtlas();
  ImFont* regularFont = nullptr;
  ImFont* monoFont = nullptr;


  // Manage some resources that we need to preserve because ImGui will use them to render at the end of the frame
  // This matters if we delete something mid-frame but have already passed a pointer to a texture for imgui to render,
  // which happens at the end of the frame.
  void preserveResourceUntilImguiFrameCompletes(std::shared_ptr<TextureBuffer> texture);

protected:
  // TODO Manage a cache of compiled shaders?

  // Render state
  int ssaaFactor = 1;
  bool enableFXAA = true;
  glm::vec4 currViewport; // TODO remove global viewport size. There is no reason for this, and stops us from doing
                          // screenshot renders while minimized.
  glm::ivec4 currScissor{0, 0, 0, 0};
  bool scissorEnabled = false;
  float currPixelScale;
  TransparencyMode transparencyMode = TransparencyMode::None;
  bool peelEnabled = true;
  int slicePlaneCount = 0;
  bool frontFaceCCW = true;
  struct SavedFramebufferState {
    FrameBuffer* framebuffer;
    glm::vec4 viewport;
    glm::ivec4 scissor;
    bool scissorEnabled;
  };
  std::vector<SavedFramebufferState> renderFramebufferStack; // supports push/popBindFramebufferForRendering
  bool useAltDisplayBuffer = false;

  // Scene & pick pipeline framebuffers, attachments, and resolve programs
  std::shared_ptr<FrameBuffer> sceneBuffer, sceneBufferFinal;
  std::shared_ptr<FrameBuffer> sceneDepthMinFrame;
  std::shared_ptr<FrameBuffer> pickFramebuffer;

  std::shared_ptr<TextureBuffer> sceneColor, sceneDepth, sceneDepthMin, sceneColorFinal;
  std::shared_ptr<RenderBuffer> pickColorBuffer, pickDepthBuffer;

  std::shared_ptr<ShaderProgram> compositePeel, mapLight, copyDepth;

  // Cached lazy settings for the resolve and relight program
  int currLightingSampleLevel = -1;
  TransparencyMode currLightingTransparencyMode = TransparencyMode::None;

  // Internal pipeline pass helpers
  void updateMinDepthTexture();
  void applyLightingTransform(std::shared_ptr<TextureBuffer>& texture);
  bool renderPickInActiveViewport(int viewportW, int viewportH, const std::function<void()>& drawPickFn);

  // ImGui/fonts
  bool imguiInitialized = false;
  ImFontAtlas* sharedFontAtlas;

  // Helpers
  virtual void freeAllOwnedResources(); // child callers should call parent
  void loadDefaultMaterials();
  void loadDefaultMaterial(std::string name);
  std::shared_ptr<TextureBuffer> loadMaterialTexture(float* data, int width, int height);
  void loadDefaultColorMap(std::string name);
  void loadDefaultColorMaps();
  virtual void createSlicePlaneFliterRule(std::string name) = 0;

  // Manage a unique ID, incremented on lots of operations. Used to distinguish updates to buffers/shaders/etc
  uint64_t uniqueID = 500;

  // Default rule lists (see enum for explanation)
  std::vector<std::string> defaultRules_sceneObject{"GLSL_VERSION", "GLOBAL_FRAGMENT_FILTER"};
  std::vector<std::string> defaultRules_pick{"GLSL_VERSION", "GLOBAL_FRAGMENT_FILTER", "SHADE_COLOR", "LIGHT_PASSTHRU"};
  std::vector<std::string> defaultRules_process{"GLSL_VERSION"};

  // Lists of points to support preserving resources until the end of an ImGUI frame (see note above)
  void clearResourcesPreservedForImguiFrame();
  std::vector<std::shared_ptr<TextureBuffer>> resourcesPreservedForImGuiFrame;
};


// === Public API
// Callers should basically only interact via these methods and variables

// The global render engine
// Gets initialized by initializeRenderEngine() in polyscope::init();
extern Engine* engine;

// The backend type of the engine, as initialized above
extern std::string engineBackendName;

// Call once to initialize
// (see render/initialize_backend.cpp)
void initializeRenderEngine(std::string backend = "");

// Get the backend name above
inline std::string getRenderEngineBackendName() { return engineBackendName; }


} // namespace render

// === Small free helpers
std::vector<std::string> removeRule(std::vector<std::string> initRules, std::string ruleName);

} // namespace polyscope

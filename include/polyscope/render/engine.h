// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "polyscope/render/color_maps.h"
#include "polyscope/render/ground_plane.h"
#include "polyscope/render/materials.h"
#include "polyscope/types.h"
#include "polyscope/view.h"

#include "imgui.h"

namespace polyscope {

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
  IndexedLineStripAdjacency
};

enum class FilterMode { Nearest = 0, Linear };
enum class TextureFormat { RGB8 = 0, RGBA8, RG16F, RGB16F, RGBA16F, RGBA32F, RGB32F, R32F, R16F, DEPTH24 };
enum class RenderBufferType { Color, ColorAlpha, Depth, Float4 };
enum class DepthMode { Less, LEqual, LEqualReadOnly, Greater, Disable, PassReadOnly };
enum class BlendMode { Over, AlphaOver, OverNoWrite, Under, Zero, WeightedAdd, Source, Disable };
enum class RenderDataType {
  Vector2Float,
  Vector3Float,
  Vector4Float,
  Matrix44Float,
  Float,
  Int,
  UInt,
  Index,
  Vector2UInt,
  Vector3UInt,
  Vector4UInt
};

int dimension(const TextureFormat& x);
std::string modeName(const TransparencyMode& m);
std::string renderDataTypeName(const RenderDataType& r);
int renderDataTypeCountCompatbility(const RenderDataType r1, const RenderDataType r2);
std::string getImageOriginRule(ImageOrigin imageOrigin);

namespace render {

class AttributeBuffer {
public:
  AttributeBuffer(RenderDataType dataType_, int arrayCount);

  virtual ~AttributeBuffer();

  virtual void setData(const std::vector<glm::vec2>& data) = 0;
  virtual void setData(const std::vector<glm::vec3>& data) = 0;
  virtual void setData(const std::vector<glm::vec4>& data) = 0;
  virtual void setData(const std::vector<float>& data) = 0;
  virtual void setData(const std::vector<double>& data) = 0;
  virtual void setData(const std::vector<int32_t>& data) = 0;
  virtual void setData(const std::vector<uint32_t>& data) = 0;
  virtual void setData(const std::vector<glm::uvec2>& data) = 0;
  virtual void setData(const std::vector<glm::uvec3>& data) = 0;
  virtual void setData(const std::vector<glm::uvec4>& data) = 0;

  // Array-valued attributes
  // (adding these lazily as we need them)
  // (sadly we cannot template the virtual function)
  virtual void setData(const std::vector<std::array<glm::vec3, 2>>& data) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 3>>& data) = 0;
  virtual void setData(const std::vector<std::array<glm::vec3, 4>>& data) = 0;

  virtual uint32_t getNativeBufferID() = 0; // used to interop with external things, e.g. ImGui

  // == Getters
  RenderDataType getType() const { return dataType; }
  int getArrayCount() const { return arrayCount; }
  int64_t getDataSize() const { return dataSize; }
  uint64_t getUniqueID() const { return uniqueID; }
  bool isSet() const { return dataSize > 0; }

  // get data at a single index from the buffer
  virtual float getData_float(size_t ind) = 0;
  virtual double getData_double(size_t ind) = 0;
  virtual glm::vec2 getData_vec2(size_t ind) = 0;
  virtual glm::vec3 getData_vec3(size_t ind) = 0;
  virtual glm::vec4 getData_vec4(size_t ind) = 0;
  virtual int getData_int(size_t ind) = 0;
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
  virtual std::vector<uint32_t> getDataRange_uint32(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec2> getDataRange_uvec2(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec3> getDataRange_uvec3(size_t ind, size_t count) = 0;
  virtual std::vector<glm::uvec4> getDataRange_uvec4(size_t ind, size_t count) = 0;

protected:
  RenderDataType dataType;
  int arrayCount;
  int64_t dataSize = -1; // the size of the data currently stored in this attribute (-1 if nothing)
  uint64_t uniqueID;
};

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
  unsigned int getTotalSize() const; // product of dimensions
  uint64_t getUniqueID() const { return uniqueID; }

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

  virtual void* getNativeHandle() = 0; // used to interop with external things, e.g. ImGui

protected:
  int dim;
  TextureFormat format;
  unsigned int sizeX, sizeY;
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
  virtual void blitTo(FrameBuffer* other) = 0;
  virtual std::vector<unsigned char> readBuffer() = 0;

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
  const int dim;
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
  SceneObject, // an object in the scene, which gets lit via matcap (etc)
  Pick,        // rendering to a pick buffer
  Process,     // postprocessing effects, etc
  None         // no defaults applied
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
  virtual void setUniform(std::string name, glm::uvec2 val) = 0;
  virtual void setUniform(std::string name, glm::uvec3 val) = 0;
  virtual void setUniform(std::string name, glm::uvec4 val) = 0;

  // = Attributes
  // clang-format off
  virtual bool hasAttribute(std::string name) = 0;
  virtual bool attributeIsSet(std::string name) = 0;
  virtual std::shared_ptr<AttributeBuffer> getAttributeBuffer(std::string name) = 0;
  virtual void setAttribute(std::string name, std::shared_ptr<AttributeBuffer> externalBuffer) = 0; 
  virtual void setAttribute(std::string name, const std::vector<glm::vec2>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec3>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<glm::vec4>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<float>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<double>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<int32_t>& data) = 0;
  virtual void setAttribute(std::string name, const std::vector<uint32_t>& data) = 0;
  // clang-format on

  // Convenience method to set an array-valued attrbute, such as 'in vec3 vertexVal[3]'. Applies interleaving then
  // forwards to the usual setAttribute
  template <typename T, unsigned int C>
  void setAttribute(std::string name, const std::vector<std::array<T, C>>& data);


  // Textures
  virtual bool hasTexture(std::string name) = 0;
  virtual bool textureIsSet(std::string name) = 0;
  virtual void setTexture1D(std::string name, unsigned char* texData, unsigned int length) = 0;
  virtual void setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                            bool withAlpha = true, bool useMipMap = false, bool repeat = false) = 0;
  virtual void setTextureFromColormap(std::string name, const std::string& colorMap, bool allowUpdate = false) = 0;
  virtual void setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer) = 0;


  // Indices
  virtual void setIndex(std::vector<std::array<unsigned int, 3>>& indices) = 0;
  virtual void setIndex(std::vector<unsigned int>& indices) = 0;
  virtual void setIndex(std::vector<glm::uvec3>& indices) = 0;
  virtual void setPrimitiveRestartIndex(unsigned int restartIndex) = 0;

  // Call once to initialize GLSL code used by multiple shaders
  static void initCommonShaders(); // TODO

  // Draw!
  virtual void draw() = 0;

  virtual void validateData() = 0;

  uint64_t getUniqueID() const { return uniqueID; }

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
  uint64_t uniqueID;
};


class Engine {

public:
  // Options

  // High-level control
  virtual void checkError(bool fatal = false) = 0;
  void buildEngineGui();

  virtual void clearDisplay();
  virtual void bindDisplay();
  virtual void swapDisplayBuffers() = 0;
  void pushBindFramebufferForRendering(
      FrameBuffer& f); // push the existing rendering framebuffer on to a stack and bind to f for rendering
  void popBindFramebufferForRendering(); // pop the old framebuffer off the stack and bind to it
  virtual std::vector<unsigned char> readDisplayBuffer() = 0;

  virtual void clearSceneBuffer();
  virtual bool bindSceneBuffer();
  virtual void resizeScreenBuffers(); // applies to all buffers tied to display size
  virtual void setScreenBufferViewports();
  virtual void
  applyLightingTransform(std::shared_ptr<TextureBuffer>& texture); // tonemap and gamma correct, render to active buffer
  void updateMinDepthTexture();
  void renderBackground(); // respects background setting

  // Manage render state
  virtual void setDepthMode(DepthMode newMode = DepthMode::Less) = 0;
  virtual void setBlendMode(BlendMode newMode = BlendMode::Over) = 0;
  virtual void setColorMask(std::array<bool, 4> mask = {true, true, true, true}) = 0;
  virtual void setBackfaceCull(bool newVal = false) = 0;

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

  // === Scene data and niceties
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

  // ImGui
  virtual void initializeImGui() = 0;
  virtual void shutdownImGui() = 0;
  void setImGuiStyle();
  ImFontAtlas* getImGuiGlobalFontAtlas();
  virtual void ImGuiNewFrame() = 0;
  virtual void ImGuiRender() = 0;
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

  // create render buffers
  virtual std::shared_ptr<RenderBuffer> generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                             unsigned int sizeY_) = 0;
  // create frame buffers
  virtual std::shared_ptr<FrameBuffer> generateFrameBuffer(unsigned int sizeX_, unsigned int sizeY_) = 0;

  // == create shader programs
  virtual std::shared_ptr<ShaderProgram>
  requestShader(const std::string& programName, const std::vector<std::string>& customRules,
                ShaderReplacementDefaults defaults = ShaderReplacementDefaults::SceneObject) = 0;

  // === The frame buffers used in the rendering pipeline
  // The size of these buffers is always kept in sync with the screen size
  std::shared_ptr<FrameBuffer> displayBuffer, displayBufferAlt;
  std::shared_ptr<FrameBuffer> sceneBuffer, sceneBufferFinal;
  std::shared_ptr<FrameBuffer> pickFramebuffer;
  std::shared_ptr<FrameBuffer> sceneDepthMinFrame;

  // Main buffers for rendering
  // sceneDepthMin is an optional texture copy of the depth buffe used for some effects
  std::shared_ptr<TextureBuffer> sceneColor, sceneColorFinal, sceneDepth, sceneDepthMin;
  std::shared_ptr<RenderBuffer> pickColorBuffer, pickDepthBuffer;

  // General-use programs used by the engine
  std::shared_ptr<ShaderProgram> renderTexturePlain, renderTextureDot3, renderTextureMap3, renderTextureSphereBG;
  std::shared_ptr<ShaderProgram> compositePeel, mapLight, copyDepth;

  // Manage transparency and culling
  void setTransparencyMode(TransparencyMode newMode);
  TransparencyMode getTransparencyMode();
  bool transparencyEnabled();
  virtual void applyTransparencySettings() = 0;
  void addSlicePlane(std::string uniquePostfix);
  void removeSlicePlane(std::string uniquePostfix);
  bool slicePlanesEnabled();                     // true if there is at least one slice plane in the scene
  virtual void setFrontFaceCCW(bool newVal) = 0; // true if CCW triangles are considered front-facing; false otherwise
  bool getFrontFaceCCW();

  // == Options
  BackgroundView background = BackgroundView::None;

  float exposure = 1.0;
  float whiteLevel = 0.75;
  float gamma = 2.2;

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

  // Helpers
  std::vector<glm::vec3> screenTrianglesCoords(); // two triangles which cover the screen
  std::vector<glm::vec4> distantCubeCoords();     // cube with vertices at infinity

  uint64_t getNextUniqueID();

  // ==  Implementation details and hacks
  bool lightCopy = false; // if true, when applying lighting transform does a copy instead of an alpha blend. Used
                          // internally for alpha in screenshots, but should generally be left as false.

  bool useAltDisplayBuffer = false; // if true, push final render results offscreen to the alt buffer instead

  // Internal windowing and engine details
  ImFontAtlas* globalFontAtlas = nullptr;
  ImFont* regularFont = nullptr;
  ImFont* monoFont = nullptr;
  FrameBuffer* currRenderFramebuffer = nullptr;

protected:
  // TODO Manage a cache of compiled shaders?

  // Render state
  int ssaaFactor = 1;
  bool enableFXAA = true;
  glm::vec4 currViewport; // TODO remove global viewport size. There is no reason for this, and stops us from doing
                          // screenshot renders while minimized.
  float currPixelScale;
  TransparencyMode transparencyMode = TransparencyMode::None;
  int slicePlaneCount = 0;
  bool frontFaceCCW = true;
  std::vector<FrameBuffer*> renderFramebufferStack; // supports push/popBindFramebufferForRendering

  // Cached lazy seettings for the resolve and relight program
  int currLightingSampleLevel = -1;
  TransparencyMode currLightingTransparencyMode = TransparencyMode::None;

  // Helpers
  void configureImGui();
  void loadDefaultMaterials();
  void loadDefaultMaterial(std::string name);
  std::shared_ptr<TextureBuffer> loadMaterialTexture(float* data, int width, int height);
  void loadDefaultColorMap(std::string name);
  void loadDefaultColorMaps();
  virtual void createSlicePlaneFliterRule(std::string name) = 0;

  // Manage a unique ID, incremented on lots of operations. Used to distinguish updates to buffers/shaders/etc
  uint64_t uniqueID = 500;

  // Default rule lists (see enum for explanation)
  std::vector<std::string> defaultRules_sceneObject{"GLSL_VERSION", "GLOBAL_FRAGMENT_FILTER", "LIGHT_MATCAP"};
  std::vector<std::string> defaultRules_pick{"GLSL_VERSION", "GLOBAL_FRAGMENT_FILTER", "SHADE_COLOR", "LIGHT_PASSTHRU"};
  std::vector<std::string> defaultRules_process{"GLSL_VERSION"};
};


// Implementation of template functions
template <typename T, unsigned int C>
inline void ShaderProgram::setAttribute(std::string name, const std::vector<std::array<T, C>>& data) {

  // Unpack and forward
  std::vector<T> entryData;
  entryData.reserve(C * data.size());
  for (auto& x : data) {
    for (size_t i = 0; i < C; i++) {
      entryData.push_back(x[i]);
    }
  }
  setAttribute(name, entryData);
}

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
} // namespace polyscope

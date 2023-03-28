// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/render/engine.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/colormap_defs.h"
#include "polyscope/render/material_defs.h"

#include "imgui.h"
#include "stb_image.h"

namespace polyscope {

int dimension(const TextureFormat& x) {
  // clang-format off
  switch (x) {
    case TextureFormat::RGB8:     return 3;
    case TextureFormat::RGBA8:    return 4;
    case TextureFormat::RG16F:    return 2;
    case TextureFormat::RGB16F:   return 3;
    case TextureFormat::RGBA16F:  return 4;
    case TextureFormat::R32F:     return 1;
    case TextureFormat::R16F:     return 1;
    case TextureFormat::RGB32F:   return 3;
    case TextureFormat::RGBA32F:  return 4;
    case TextureFormat::DEPTH24:  return 1;
  }
  // clang-format on
  exception("bad enum");
  return 0;
}

std::string renderDataTypeName(const RenderDataType& r) {
  switch (r) {
  case RenderDataType::Vector2Float:
    return "Vector2Float";
  case RenderDataType::Vector3Float:
    return "Vector3Float";
  case RenderDataType::Vector4Float:
    return "Vector4Float";
  case RenderDataType::Matrix44Float:
    return "Matrix44Float";
  case RenderDataType::Float:
    return "Float";
  case RenderDataType::Int:
    return "Int";
  case RenderDataType::UInt:
    return "UInt";
  case RenderDataType::Index:
    return "Index";
  case RenderDataType::Vector2UInt:
    return "Vector2UInt";
  case RenderDataType::Vector3UInt:
    return "Vector3UInt";
  case RenderDataType::Vector4UInt:
    return "Vector4UInt";
  }
  return "";
}

int renderDataTypeCountCompatbility(const RenderDataType r1, const RenderDataType r2) {

  if (r1 == r2) return 1;

  if (r1 == RenderDataType::Vector2Float && r2 == RenderDataType::Float) return 2;
  if (r1 == RenderDataType::Vector3Float && r2 == RenderDataType::Float) return 3;
  if (r1 == RenderDataType::Vector4Float && r2 == RenderDataType::Float) return 4;

  if (r1 == RenderDataType::Vector2UInt && r2 == RenderDataType::UInt) return 2;
  if (r1 == RenderDataType::Vector3UInt && r2 == RenderDataType::UInt) return 3;
  if (r1 == RenderDataType::Vector4UInt && r2 == RenderDataType::UInt) return 4;

  // there are other combinations of types which could be compatible, we don't handle them yet
  //
  return 0;
}

std::string modeName(const TransparencyMode& m) {
  switch (m) {
  case TransparencyMode::None:
    return "None";
  case TransparencyMode::Simple:
    return "Simple";
  case TransparencyMode::Pretty:
    return "Pretty";
  }
  return "";
}

std::string getImageOriginRule(ImageOrigin imageOrigin) {
  switch (imageOrigin) {
  case ImageOrigin::UpperLeft:
    return "TEXTURE_ORIGIN_UPPERLEFT";
    break;
  case ImageOrigin::LowerLeft:
    return "TEXTURE_ORIGIN_LOWERLEFT";
    break;
  }
  return "";
}

namespace render {

AttributeBuffer::AttributeBuffer(RenderDataType dataType_, int arrayCount_)
    : dataType(dataType_), arrayCount(arrayCount_), uniqueID(render::engine->getNextUniqueID()) {}

AttributeBuffer::~AttributeBuffer() {}

TextureBuffer::TextureBuffer(int dim_, TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_)
    : dim(dim_), format(format_), sizeX(sizeX_), sizeY(sizeY_), uniqueID(render::engine->getNextUniqueID()) {
  if (sizeX > (1 << 22)) exception("OpenGL error: invalid texture dimensions");
  if (dim > 1 && sizeY > (1 << 22)) exception("OpenGL error: invalid texture dimensions");
}

TextureBuffer::~TextureBuffer() {}

void TextureBuffer::setFilterMode(FilterMode newMode) {}

void TextureBuffer::resize(unsigned int newLen) { sizeX = newLen; }
void TextureBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}

unsigned int TextureBuffer::getTotalSize() const {
  switch (dim) {
  case 1:
    return getSizeX();
  case 2:
    return getSizeX() * getSizeY();
  case 3:
    exception("not implemented");
    return -1;
  }
  return -1;
}

RenderBuffer::RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : type(type_), sizeX(sizeX_), sizeY(sizeY_), uniqueID(render::engine->getNextUniqueID()) {
  if (sizeX > (1 << 22) || sizeY > (1 << 22)) exception("OpenGL error: invalid renderbuffer dimensions");
}

void RenderBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}

FrameBuffer::FrameBuffer() : uniqueID(render::engine->getNextUniqueID()) {}

void FrameBuffer::setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY) {
  viewportX = startX;
  viewportY = startY;
  viewportSizeX = sizeX;
  viewportSizeY = sizeY;
  viewportSet = true;
}

void FrameBuffer::resize(unsigned int newXSize, unsigned int newYSize) {
  bind();
  for (auto& b : renderBuffersColor) {
    b->resize(newXSize, newYSize);
  }
  for (auto& b : renderBuffersDepth) {
    b->resize(newXSize, newYSize);
  }
  for (auto& b : textureBuffersColor) {
    b->resize(newXSize, newYSize);
  }
  for (auto& b : textureBuffersDepth) {
    b->resize(newXSize, newYSize);
  }
  sizeX = newXSize;
  sizeY = newYSize;
}

void FrameBuffer::verifyBufferSizes() {
  for (auto& b : renderBuffersColor) {
    if (b->getSizeX() != getSizeX() || b->getSizeY() != getSizeY())
      exception("render buffer size does not match framebuffer size");
  }
}

ShaderReplacementRule::ShaderReplacementRule() {}

ShaderReplacementRule::ShaderReplacementRule(std::string ruleName_,
                                             std::vector<std::pair<std::string, std::string>> replacements_)
    : ruleName(ruleName_), replacements(replacements_) {}

ShaderReplacementRule::ShaderReplacementRule(std::string ruleName_,
                                             std::vector<std::pair<std::string, std::string>> replacements_,
                                             std::vector<ShaderSpecUniform> uniforms_,
                                             std::vector<ShaderSpecAttribute> attributes_,
                                             std::vector<ShaderSpecTexture> textures_)
    : ruleName(ruleName_), replacements(replacements_), uniforms(uniforms_), attributes(attributes_),
      textures(textures_) {}

ShaderProgram::ShaderProgram(DrawMode dm) : drawMode(dm), uniqueID(render::engine->getNextUniqueID()) {

  drawMode = dm;
  if (dm == DrawMode::IndexedLines || dm == DrawMode::IndexedLineStrip || dm == DrawMode::IndexedLineStripAdjacency ||
      dm == DrawMode::IndexedTriangles) {
    useIndex = true;
  }

  if (dm == DrawMode::IndexedLineStripAdjacency) {
    usePrimitiveRestart = true;
  }
}

void Engine::buildEngineGui() {

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Appearance")) {

    // == Display
    ImGui::PushItemWidth(120);
    // ImGui::Text("Background");
    // ImGui::SameLine();
    static std::string displayBackgroundName = "None";
    // if (ImGui::BeginCombo("##Background", displayBackgroundName.c_str())) {
    // if (ImGui::Selectable("None", background == BackgroundView::None)) {
    // background = BackgroundView::None;
    // ImGui::SetItemDefaultFocus();
    // displayBackgroundName = "None";
    //}
    // ImGui::EndCombo();
    //}

    ImGui::ColorEdit4("background color", (float*)&view::bgColor, ImGuiColorEditFlags_NoInputs);

    // == Transparency
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Transparency")) {

      if (ImGui::BeginCombo("Mode", modeName(transparencyMode).c_str())) {
        for (TransparencyMode m : {TransparencyMode::None, TransparencyMode::Simple, TransparencyMode::Pretty}) {
          std::string mName = modeName(m);
          if (ImGui::Selectable(mName.c_str(), transparencyMode == m)) {
            options::transparencyMode = m;
            requestRedraw();
          }
        }
        ImGui::EndCombo();
      }

      switch (transparencyMode) {
      case TransparencyMode::None: {
        ImGui::TextWrapped("Transparency effects are disabled and all related options are ignored.");
        break;
      }
      case TransparencyMode::Simple: {
        ImGui::TextWrapped(
            "Simple transparent rendering. Efficient, but objects at different depths may not look right.");
        break;
      }
      case TransparencyMode::Pretty: {
        ImGui::TextWrapped("Accurate but expensive transparent rendering. Increase the number of passes to resolve "
                           "complicated scenes.");
        if (ImGui::InputInt("Render Passes", &options::transparencyRenderPasses)) {
          requestRedraw();
        }
        break;
      }
      }

      ImGui::TreePop();
    }

    // == Ground plane
    groundPlane.buildGui();

    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Tone Mapping")) {
      ImGui::SliderFloat("exposure", &exposure, 0.1, 2.0, "%.3f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
      ImGui::SliderFloat("white level", &whiteLevel, 0.0, 2.0, "%.3f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
      ImGui::SliderFloat("gamma", &gamma, 0.5, 3.0, "%.3f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
      ImGui::TreePop();
    }

    // == Anti-aliasing
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Anti-Aliasing")) {
      if (ImGui::InputInt("SSAA (pretty)", &ssaaFactor, 1)) {
        ssaaFactor = std::min(ssaaFactor, 4);
        ssaaFactor = std::max(ssaaFactor, 1);
        options::ssaaFactor = ssaaFactor;
        requestRedraw();
      }
      ImGui::TreePop();
    }

    // == Materials
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Materials")) {

      ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
      if (ImGui::TreeNode("Load material")) {

        size_t buffLen = 512;
        static std::vector<char> buffName(buffLen);
        ImGui::InputText("Material name", &buffName[0], buffLen);
        static std::vector<char> buffFile(buffLen);
        ImGui::InputText("File name", &buffFile[0], buffLen);

        if (ImGui::Button("Load static material")) {
          std::string filename(&buffFile[0]);
          std::string matName(&buffName[0]);
          polyscope::loadStaticMaterial(matName, filename);
        }

        if (ImGui::Button("Load blendable material")) {
          std::string filename(&buffFile[0]);
          std::string matName(&buffName[0]);
          std::string filebase, fileext;
          std::tie(filebase, fileext) = splitExt(filename);
          polyscope::loadBlendableMaterial(matName, filebase, fileext);
        }

        ImGui::TreePop();
      }

      ImGui::TreePop();
    }

    // == Color maps
    if (ImGui::TreeNode("Color Maps")) {

      ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
      if (ImGui::TreeNode("Load color map")) {

        size_t buffLen = 512;
        static std::vector<char> buffName(buffLen);
        ImGui::InputText("Color map name", &buffName[0], buffLen);
        static std::vector<char> buffFile(buffLen);
        ImGui::InputText("File name", &buffFile[0], buffLen);

        if (ImGui::Button("Load")) {
          std::string filename(&buffFile[0]);
          std::string cmapName(&buffName[0]);
          polyscope::loadColorMap(cmapName, filename);
        }

        ImGui::TreePop();
      }

      ImGui::TreePop();
    }

    ImGui::TreePop();
  }
}

void Engine::setBackgroundColor(glm::vec3 c) {
  FrameBuffer& targetBuffer = useAltDisplayBuffer ? *displayBufferAlt : *displayBuffer;
  targetBuffer.clearColor = c;
}

void Engine::setBackgroundAlpha(float newAlpha) {
  FrameBuffer& targetBuffer = useAltDisplayBuffer ? *displayBufferAlt : *displayBuffer;
  targetBuffer.clearAlpha = newAlpha;
}

void Engine::setCurrentViewport(glm::vec4 val) { currViewport = val; }
glm::vec4 Engine::getCurrentViewport() { return currViewport; }
void Engine::setCurrentPixelScaling(float val) { currPixelScale = val; }
float Engine::getCurrentPixelScaling() { return currPixelScale; }

void Engine::bindDisplay() {
  FrameBuffer& targetBuffer = useAltDisplayBuffer ? *displayBufferAlt : *displayBuffer;
  targetBuffer.bindForRendering();
}


void Engine::clearDisplay() {
  FrameBuffer& targetBuffer = useAltDisplayBuffer ? *displayBufferAlt : *displayBuffer;
  targetBuffer.clear();
}


void Engine::clearSceneBuffer() { sceneBuffer->clear(); }

void Engine::resizeScreenBuffers() {
  unsigned int width = view::bufferWidth;
  unsigned int height = view::bufferHeight;
  displayBuffer->resize(width, height);
  displayBufferAlt->resize(width, height);
  sceneBuffer->resize(ssaaFactor * width, ssaaFactor * height);
  sceneBufferFinal->resize(ssaaFactor * width, ssaaFactor * height);
  sceneDepthMinFrame->resize(ssaaFactor * width, ssaaFactor * height);
}

void Engine::setScreenBufferViewports() {
  unsigned int xStart = 0;
  unsigned int yStart = 0;
  unsigned int sizeX = view::bufferWidth;
  unsigned int sizeY = view::bufferHeight;

  displayBuffer->setViewport(xStart, yStart, sizeX, sizeY);
  displayBufferAlt->setViewport(xStart, yStart, sizeX, sizeY);
  sceneBuffer->setViewport(ssaaFactor * xStart, ssaaFactor * yStart, ssaaFactor * sizeX, ssaaFactor * sizeY);
  sceneBufferFinal->setViewport(ssaaFactor * xStart, ssaaFactor * yStart, ssaaFactor * sizeX, ssaaFactor * sizeY);
  sceneDepthMinFrame->setViewport(ssaaFactor * xStart, ssaaFactor * yStart, ssaaFactor * sizeX, ssaaFactor * sizeY);
}

bool Engine::bindSceneBuffer() {
  setCurrentPixelScaling(ssaaFactor);
  return sceneBuffer->bindForRendering();
}

void Engine::applyLightingTransform(std::shared_ptr<TextureBuffer>& texture) {

  glm::vec4 currV = getCurrentViewport();

  // If the viewport extents are 0, don't do anything. This happens e.g. on Windows when the window is minimized.
  if (currV[2] == 0 || currV[3] == 0) {
    return;
  }

  // compute downsampling rate
  float sampleX = texture->getSizeX() / currV[2];
  float sampleY = texture->getSizeY() / currV[3];
  if (sampleX != sampleY) exception("lighting downsampling should have same aspect");
  int sampleLevel;
  if (sampleX < 1.) {
    sampleLevel = 1;
  } else {
    if (sampleX != static_cast<int>(sampleX)) exception("lighting downsampling should have integer ratio");
    sampleLevel = static_cast<int>(sampleX);
    if (sampleLevel > 4) exception("lighting downsampling only implemented up to 4x");
  }

  // == Lazily regnerate the mapper if it doesn't match the current settings
  if (!mapLight || currLightingSampleLevel != sampleLevel || currLightingTransparencyMode != transparencyMode) {

    std::string sampleRuleName = "";
    if (sampleLevel == 1) sampleRuleName = "DOWNSAMPLE_RESOLVE_1";
    if (sampleLevel == 2) sampleRuleName = "DOWNSAMPLE_RESOLVE_2";
    if (sampleLevel == 3) sampleRuleName = "DOWNSAMPLE_RESOLVE_3";
    if (sampleLevel == 4) sampleRuleName = "DOWNSAMPLE_RESOLVE_4";

    std::vector<std::string> resolveRules = {sampleRuleName};

    switch (transparencyMode) {
    case TransparencyMode::None:
      break;
    case TransparencyMode::Simple:
      resolveRules.push_back("TRANSPARENCY_RESOLVE_SIMPLE");
      break;
    case TransparencyMode::Pretty:
      break;
    }

    mapLight = render::engine->requestShader("MAP_LIGHT", resolveRules, render::ShaderReplacementDefaults::Process);
    mapLight->setAttribute("a_position", screenTrianglesCoords());
    currLightingSampleLevel = sampleLevel;
    currLightingTransparencyMode = transparencyMode;
  }

  mapLight->setUniform("u_exposure", exposure);
  mapLight->setUniform("u_whiteLevel", whiteLevel);
  mapLight->setUniform("u_gamma", gamma);
  mapLight->setTextureFromBuffer("t_image", texture.get());

  glm::vec2 texelSize{1. / texture->getSizeX(), 1. / texture->getSizeY()};
  mapLight->setUniform("u_texelSize", texelSize);

  if (lightCopy) {
    setBlendMode(BlendMode::Disable);
  } else {
    setBlendMode(BlendMode::AlphaOver);
  }
  render::engine->setDepthMode(DepthMode::Disable);
  mapLight->draw();
}

void Engine::setMaterial(ShaderProgram& program, const std::string& mat) {
  const Material& m = getMaterial(mat);
  program.setTextureFromBuffer("t_mat_r", m.textureBuffers[0].get());
  program.setTextureFromBuffer("t_mat_g", m.textureBuffers[1].get());
  program.setTextureFromBuffer("t_mat_b", m.textureBuffers[2].get());
  program.setTextureFromBuffer("t_mat_k", m.textureBuffers[3].get());
}

void Engine::renderBackground() {
  switch (background) {
  case BackgroundView::None:
    break;
    /*
      case BackgroundView::Env: {
        glm::mat4 V = view::getCameraViewMatrix();
        glm::mat4 P = view::getCameraPerspectiveMatrix();
        renderTextureSphereBG->setUniform("u_viewMatrix", glm::value_ptr(V));
        renderTextureSphereBG->setUniform("u_projMatrix", glm::value_ptr(P));
        renderTextureSphereBG->setTextureFromBuffer("t_image", envMapOrig.get());
        setDepthMode(DepthMode::LEqualReadOnly);
        renderTextureSphereBG->draw();
        break;
      }
    */
  }
}


void Engine::setTransparencyMode(TransparencyMode newMode) {
  // Remove any old transparency-related rules
  switch (transparencyMode) {
  case TransparencyMode::None: {
    break;
  }
  case TransparencyMode::Simple: {
    defaultRules_sceneObject.erase(
        std::remove(defaultRules_sceneObject.begin(), defaultRules_sceneObject.end(), "TRANSPARENCY_STRUCTURE"),
        defaultRules_sceneObject.end());
    break;
  }
  case TransparencyMode::Pretty: {
    defaultRules_sceneObject.erase(
        std::remove(defaultRules_sceneObject.begin(), defaultRules_sceneObject.end(), "TRANSPARENCY_PEEL_STRUCTURE"),
        defaultRules_sceneObject.end());
    break;
  }
  }

  transparencyMode = newMode;

  // Add a new rule for this setting
  switch (newMode) {
  case TransparencyMode::None: {
    break;
  }
  case TransparencyMode::Simple: {
    defaultRules_sceneObject.push_back("TRANSPARENCY_STRUCTURE");
    break;
  }
  case TransparencyMode::Pretty: {
    defaultRules_sceneObject.push_back("TRANSPARENCY_PEEL_STRUCTURE");
    break;
  }
  }

  // Regenerate _all_ the things
  refresh();
}

TransparencyMode Engine::getTransparencyMode() { return transparencyMode; }

bool Engine::transparencyEnabled() {
  switch (transparencyMode) {
  case TransparencyMode::None:
    return false;
  case TransparencyMode::Simple:
    return true;
  case TransparencyMode::Pretty:
    return true;
  }
  return false;
}

void Engine::setSSAAFactor(int newVal) {
  if (newVal < 1 || newVal > 4) exception("ssaaFactor must be one of 1,2,3,4");
  ssaaFactor = newVal;
  updateWindowSize(true);
}

bool Engine::getFrontFaceCCW() { return frontFaceCCW; }

int Engine::getSSAAFactor() { return ssaaFactor; }

void Engine::allocateGlobalBuffersAndPrograms() {

  // Note: The display frame buffer should be manually wrapped by child classes

  { // Scene buffer

    // Note that this is basically duplicated in ground_plane.cpp, changes here should probably be reflected there
    sceneColor = generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
    // sceneDepth = generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);
    sceneDepth = generateTextureBuffer(TextureFormat::DEPTH24, view::bufferWidth, view::bufferHeight);

    sceneBuffer = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    sceneBuffer->addColorBuffer(sceneColor);
    sceneBuffer->addDepthBuffer(sceneDepth);
    sceneBuffer->setDrawBuffers();

    sceneBuffer->clearColor = glm::vec3{1., 1., 1.};
    sceneBuffer->clearAlpha = 0.0;
  }

  { // Alternate depth texture used for some effects
    sceneDepthMin = generateTextureBuffer(TextureFormat::DEPTH24, view::bufferWidth, view::bufferHeight);

    sceneDepthMinFrame = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    sceneDepthMinFrame->addDepthBuffer(sceneDepthMin);
    sceneDepthMinFrame->clearDepth = 0.0;
  }

  { // "Final" scene buffer (after resolving)
    sceneColorFinal = generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);

    sceneBufferFinal = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    sceneBufferFinal->addColorBuffer(sceneColorFinal);
    sceneBufferFinal->setDrawBuffers();

    sceneBufferFinal->clearColor = glm::vec3{1., 1., 1.};
    sceneBufferFinal->clearAlpha = 0.0;
  }

  { // Alternate display buffer
    std::shared_ptr<RenderBuffer> sceneColorAlt =
        generateRenderBuffer(RenderBufferType::ColorAlpha, view::bufferWidth, view::bufferHeight);
    std::shared_ptr<RenderBuffer> sceneDepthAlt =
        generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);

    displayBufferAlt = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    displayBufferAlt->addColorBuffer(sceneColorAlt);
    displayBufferAlt->addDepthBuffer(sceneDepthAlt);
    displayBufferAlt->setDrawBuffers();

    displayBufferAlt->clearColor = glm::vec3{1., 1., 1.};
    displayBufferAlt->clearAlpha = 0.0;
  }

  { // Pick buffer
    pickColorBuffer = generateRenderBuffer(RenderBufferType::Float4, view::bufferWidth, view::bufferHeight);
    pickDepthBuffer = generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);

    pickFramebuffer = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    pickFramebuffer->addColorBuffer(pickColorBuffer);
    pickFramebuffer->addDepthBuffer(pickDepthBuffer);
    pickFramebuffer->setDrawBuffers();
  }

  // Make sure all the buffer sizes are up to date
  updateWindowSize(true);

  { // Generate the general-use programs
    // clang-format off
    renderTexturePlain = render::engine->requestShader("TEXTURE_DRAW_PLAIN", {}, render::ShaderReplacementDefaults::Process);
    renderTexturePlain->setAttribute("a_position", screenTrianglesCoords());

    renderTextureDot3 = render::engine->requestShader("TEXTURE_DRAW_DOT3", {}, render::ShaderReplacementDefaults::Process);
    renderTextureDot3->setAttribute("a_position", screenTrianglesCoords());

    renderTextureMap3 = render::engine->requestShader("TEXTURE_DRAW_MAP3", {}, render::ShaderReplacementDefaults::Process);
    renderTextureMap3->setAttribute("a_position", screenTrianglesCoords());

    renderTextureSphereBG = render::engine->requestShader("TEXTURE_DRAW_SPHEREBG", {}, render::ShaderReplacementDefaults::Process);
    renderTextureSphereBG->setAttribute("a_position", distantCubeCoords());

    compositePeel = render::engine->requestShader("COMPOSITE_PEEL", {}, render::ShaderReplacementDefaults::Process);
    compositePeel->setAttribute("a_position", screenTrianglesCoords());
    compositePeel->setTextureFromBuffer("t_image", sceneColor.get());

    copyDepth = render::engine->requestShader("DEPTH_COPY", {}, render::ShaderReplacementDefaults::Process);
    copyDepth->setAttribute("a_position", screenTrianglesCoords());
    copyDepth->setTextureFromBuffer("t_depth", sceneDepth.get());
    // clang-format on
  }

  { // Load defaults
    loadDefaultMaterials();
    loadDefaultColorMaps();
  }
}

uint64_t Engine::getNextUniqueID() {
  uint64_t thisID = uniqueID;
  uniqueID++;
  return thisID;
}

void Engine::pushBindFramebufferForRendering(FrameBuffer& f) {
  if (currRenderFramebuffer == nullptr) exception("tried to push current framebuff on to stack, but it is null");
  renderFramebufferStack.push_back(currRenderFramebuffer);
  f.bindForRendering();
}

void Engine::popBindFramebufferForRendering() {
  if (renderFramebufferStack.empty())
    exception("called popBindFramebufferForRendering() on empty stack. Forgot to push?");
  renderFramebufferStack.back()->bindForRendering();
  renderFramebufferStack.pop_back();
}

void Engine::addSlicePlane(std::string uniquePostfix) {

  // NOTE: Unfortunately, the logic here and in slice_plane.cpp depends on the names constructed from the postfix being
  // identical.

  createSlicePlaneFliterRule(uniquePostfix);
  slicePlaneCount++;

  // Add rules
  std::vector<std::string> newRules{"SLICE_PLANE_CULL_" + uniquePostfix};
  defaultRules_sceneObject.insert(defaultRules_sceneObject.end(), newRules.begin(), newRules.end());
  defaultRules_pick.insert(defaultRules_pick.end(), newRules.begin(), newRules.end());

  // Regenerate everything
  polyscope::refresh();
}

void Engine::removeSlicePlane(std::string uniquePostfix) {

  slicePlaneCount--;
  // Remove the (last occurence of the) rules we added
  std::vector<std::string> newRules{"SLICE_PLANE_CULL_" + uniquePostfix};
  auto deleteLast = [&](std::vector<std::string>& vec, std::string target) {
    for (size_t i = vec.size(); i > 0; i--) {
      if (vec[i - 1] == target) {
        vec.erase(vec.begin() + (i - 1));
        return;
      }
    }
  };
  for (std::string r : newRules) {
    deleteLast(defaultRules_sceneObject, r);
    deleteLast(defaultRules_pick, r);
  }

  // Don't bother undoing the createRule(), since it doesn't really hurt to leave it around

  // Regenerate everything
  polyscope::refresh();
}

bool Engine::slicePlanesEnabled() { return slicePlaneCount > 0; }


std::vector<glm::vec3> Engine::screenTrianglesCoords() {
  std::vector<glm::vec3> coords = {{-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f},
                                   {-1.0f, 1.0f, 0.0f},  {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};
  return coords;
}

std::vector<glm::vec4> Engine::distantCubeCoords() {
  std::vector<glm::vec4> coords;

  auto addCubeFace = [&](int iS, float s) {
    int iU = (iS + 1) % 3;
    int iR = (iS + 2) % 3;

    glm::vec4 lowerLeft(0., 0., 0., 0.);
    lowerLeft[iS] = s;
    lowerLeft[iU] = -s;
    lowerLeft[iR] = -s;

    glm::vec4 lowerRight(0., 0., 0., 0.);
    lowerRight[iS] = s;
    lowerRight[iU] = -s;
    lowerRight[iR] = s;

    glm::vec4 upperLeft(0., 0., 0., 0.);
    upperLeft[iS] = s;
    upperLeft[iU] = s;
    upperLeft[iR] = -s;

    glm::vec4 upperRight(0., 0., 0., 0.);
    upperRight[iS] = s;
    upperRight[iU] = s;
    upperRight[iR] = s;

    // first triangle
    coords.push_back(lowerLeft);
    coords.push_back(lowerRight);
    coords.push_back(upperRight);

    // second triangle
    coords.push_back(lowerLeft);
    coords.push_back(upperRight);
    coords.push_back(upperLeft);
  };

  addCubeFace(0, +1.);
  addCubeFace(0, -1.);
  addCubeFace(1, +1.);
  addCubeFace(1, -1.);
  addCubeFace(2, +1.);
  addCubeFace(2, -1.);

  return coords;
}

void Engine::updateMinDepthTexture() {
  setDepthMode(DepthMode::Greater);
  sceneDepthMinFrame->bind();
  copyDepth->draw();
}


// Helper (TODO rework to load custom materials)
void Engine::loadDefaultMaterial(std::string name) {

  Material* newMaterial = new Material();
  newMaterial->name = name;

  std::array<unsigned char const*, 4> buff;
  std::array<size_t, 4> buffSize;

  // clang-format off
  if(name == "clay") {
    newMaterial->supportsRGB = true;
    buff[0] = &bindata_clay_r[0]; buffSize[0] = bindata_clay_r.size();
    buff[1] = &bindata_clay_g[0]; buffSize[1] = bindata_clay_g.size();
    buff[2] = &bindata_clay_b[0]; buffSize[2] = bindata_clay_b.size();
    buff[3] = &bindata_clay_k[0]; buffSize[3] = bindata_clay_k.size();
  }
  else if(name == "wax") {
    newMaterial->supportsRGB = true;
    buff[0] = &bindata_wax_r[0]; buffSize[0] = bindata_wax_r.size();
    buff[1] = &bindata_wax_g[0]; buffSize[1] = bindata_wax_g.size();
    buff[2] = &bindata_wax_b[0]; buffSize[2] = bindata_wax_b.size();
    buff[3] = &bindata_wax_k[0]; buffSize[3] = bindata_wax_k.size();
  }
  else if(name == "candy") {
    newMaterial->supportsRGB = true;
    buff[0] = &bindata_candy_r[0]; buffSize[0] = bindata_candy_r.size();
    buff[1] = &bindata_candy_g[0]; buffSize[1] = bindata_candy_g.size();
    buff[2] = &bindata_candy_b[0]; buffSize[2] = bindata_candy_b.size();
    buff[3] = &bindata_candy_k[0]; buffSize[3] = bindata_candy_k.size();
  }
  else if(name == "flat") {
    newMaterial->supportsRGB = true;
    buff[0] = &bindata_flat_r[0]; buffSize[0] = bindata_flat_r.size();
    buff[1] = &bindata_flat_g[0]; buffSize[1] = bindata_flat_g.size();
    buff[2] = &bindata_flat_b[0]; buffSize[2] = bindata_flat_b.size();
    buff[3] = &bindata_flat_k[0]; buffSize[3] = bindata_flat_k.size();
  } 
  else if(name == "mud") {
    newMaterial->supportsRGB = false;
    for(int i = 0; i < 4; i++) {buff[i] = &bindata_mud[0]; buffSize[i] = bindata_mud.size();}
	}
  else if(name == "ceramic") {
    newMaterial->supportsRGB = false;
    for(int i = 0; i < 4; i++) {buff[i] = &bindata_ceramic[0]; buffSize[i] = bindata_ceramic.size();}
	}
  else if(name == "jade") {
    newMaterial->supportsRGB = false;
    for(int i = 0; i < 4; i++) {buff[i] = &bindata_jade[0]; buffSize[i] = bindata_jade.size();}
	}
  else if(name == "normal") {
    newMaterial->supportsRGB = false;
    for(int i = 0; i < 4; i++) {buff[i] = &bindata_normal[0]; buffSize[i] = bindata_normal.size();}
	} else {
    exception("unrecognized default material name " + name);
  }
  // clang-format on

  for (int i = 0; i < 4; i++) {
    int width, height, nComp;
    float* data = stbi_loadf_from_memory(buff[i], buffSize[i], &width, &height, &nComp, 3);
    if (!data) exception("failed to load material");
    newMaterial->textureBuffers[i] = loadMaterialTexture(data, width, height);
    stbi_image_free(data);
  }

  materials.emplace_back(newMaterial);
}

void Engine::loadBlendableMaterial(std::string matName, std::array<std::string, 4> filenames) {

  for (auto& m : materials) {
    if (m->name == matName) {
      polyscope::warning("material named " + matName + " already exists");
      return;
    }
  }

  Material* newMaterial = new Material();
  newMaterial->name = matName;
  newMaterial->supportsRGB = true;
  materials.emplace_back(newMaterial);

  // Load each of the four components
  for (int i = 0; i < 4; i++) {
    int width, height, nComp;
    float* data = stbi_loadf(filenames[i].c_str(), &width, &height, &nComp, 3);
    if (!data) {
      polyscope::warning("failed to load material from " + filenames[i]);
      materials.pop_back();
      return;
    }
    newMaterial->textureBuffers[i] = loadMaterialTexture(data, width, height);
    stbi_image_free(data);
  }
}

void Engine::loadStaticMaterial(std::string matName, std::string filename) {

  for (auto& m : materials) {
    if (m->name == matName) {
      polyscope::warning("material named " + matName + " already exists");
      return;
    }
  }

  Material* newMaterial = new Material();
  newMaterial->name = matName;
  newMaterial->supportsRGB = false;
  materials.emplace_back(newMaterial);

  // Load each of the four components
  for (int i = 0; i < 4; i++) {
    int width, height, nComp;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &nComp, 3);
    if (!data) {
      polyscope::warning("failed to load material from " + filename);
      materials.pop_back();
      return;
    }
    newMaterial->textureBuffers[i] = loadMaterialTexture(data, width, height);
    stbi_image_free(data);
  }
}

void Engine::loadBlendableMaterial(std::string matName, std::string filenameBase, std::string filenameExt) {

  std::array<std::string, 4> names = {filenameBase + "_r" + filenameExt, filenameBase + "_g" + filenameExt,
                                      filenameBase + "_b" + filenameExt, filenameBase + "_k" + filenameExt};
  loadBlendableMaterial(matName, names);
}

std::shared_ptr<TextureBuffer> Engine::loadMaterialTexture(float* data, int width, int height) {
  std::shared_ptr<TextureBuffer> t = engine->generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
  t->setFilterMode(FilterMode::Linear);
  return t;
}

void Engine::loadDefaultMaterials() {
  loadDefaultMaterial("clay");
  loadDefaultMaterial("wax");
  loadDefaultMaterial("candy");
  loadDefaultMaterial("flat");
  loadDefaultMaterial("mud");
  loadDefaultMaterial("ceramic");
  loadDefaultMaterial("jade");
  loadDefaultMaterial("normal");
}


Material& Engine::getMaterial(const std::string& name) {
  for (std::unique_ptr<Material>& m : materials) {
    if (name == m->name) return *m;
  }

  exception("unrecognized material name: " + name);
  return *materials[0];
}

void Engine::loadColorMap(std::string cmapName, std::string filename) {

  for (auto& cmap : colorMaps) {
    if (cmapName == cmap->name) {
      polyscope::warning("color map named " + cmapName + " already exists");
    }
  }

  // Load the image
  int width, height, nComp;
  unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nComp, 3);
  if (!data) {
    polyscope::warning("failed to load colormap from " + filename);
    return;
  }

  // Parse the data in to a float array
  // (assumes colormap is oriented horizontally)
  std::vector<glm::vec3> vals;
  int readRow = height / 2;
  for (int iCol = 0; iCol < width; iCol++) {
    int pixInd = (readRow * width + iCol) * 3;
    unsigned char pR = data[pixInd + 0];
    unsigned char pG = data[pixInd + 1];
    unsigned char pB = data[pixInd + 2];
    glm::vec3 val = {pR / 255., pG / 255., pB / 255.};
    vals.push_back(val);
  }

  stbi_image_free(data);

  ValueColorMap* newMap = new ValueColorMap();
  newMap->name = cmapName;
  newMap->values = vals;
  colorMaps.emplace_back(newMap);
}

const ValueColorMap& Engine::getColorMap(const std::string& name) {
  for (auto& cmap : colorMaps) {
    if (name == cmap->name) return *cmap;
  }

  exception("unrecognized colormap name: " + name);
  return *colorMaps[0];
}


void Engine::configureImGui() {

  if (options::prepareImGuiFontsCallback) {
    std::tie(globalFontAtlas, regularFont, monoFont) = options::prepareImGuiFontsCallback();
  }


  if (options::configureImGuiStyleCallback) {
    options::configureImGuiStyleCallback();
  }
}

void Engine::loadDefaultColorMap(std::string name) {

  const std::vector<glm::vec3>* buff = nullptr;
  if (name == "viridis") {
    buff = &CM_VIRIDIS;
  } else if (name == "coolwarm") {
    buff = &CM_COOLWARM;
  } else if (name == "blues") {
    buff = &CM_BLUES;
  } else if (name == "reds") {
    buff = &CM_REDS;
  } else if (name == "pink-green") {
    buff = &CM_PIYG;
  } else if (name == "phase") {
    buff = &CM_PHASE;
  } else if (name == "spectral") {
    buff = &CM_SPECTRAL;
  } else if (name == "rainbow") {
    buff = &CM_RAINBOW;
  } else if (name == "jet") {
    buff = &CM_JET;
  } else if (name == "turbo") {
    buff = &CM_TURBO;
  } else {
    exception("unrecognized default colormap " + name);
  }

  ValueColorMap* newMap = new ValueColorMap();
  newMap->name = name;
  newMap->values = *buff;
  colorMaps.emplace_back(newMap);
}

void Engine::loadDefaultColorMaps() {
  loadDefaultColorMap("viridis");
  loadDefaultColorMap("coolwarm");
  loadDefaultColorMap("blues");
  loadDefaultColorMap("reds");
  loadDefaultColorMap("pink-green");
  loadDefaultColorMap("phase");
  loadDefaultColorMap("spectral");
  loadDefaultColorMap("rainbow");
  loadDefaultColorMap("jet");
  loadDefaultColorMap("turbo");
}


void Engine::showTextureInImGuiWindow(std::string windowName, TextureBuffer* buffer) {
  ImGui::Begin(windowName.c_str());

  if (buffer->getDimension() != 2) exception("only know how to show 2D textures");

  float w = ImGui::GetWindowWidth();
  float h = w * buffer->getSizeY() / buffer->getSizeX();

  ImGui::Text("Dimensions: %dx%d", buffer->getSizeX(), buffer->getSizeY());
  ImGui::Image(buffer->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

  ImGui::End();
}

ImFontAtlas* Engine::getImGuiGlobalFontAtlas() { return globalFontAtlas; }

} // namespace render
} // namespace polyscope

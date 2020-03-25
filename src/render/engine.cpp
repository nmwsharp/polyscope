// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/engine.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/colormap_defs.h"
#include "polyscope/render/material_defs.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"
#include "stb_image.h"


namespace polyscope {
namespace render {

TextureBuffer::TextureBuffer(int dim_, TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_)
    : dim(dim_), format(format_), sizeX(sizeX_), sizeY(sizeY_) {
  if (sizeX > (1 << 22)) throw std::runtime_error("OpenGL error: invalid texture dimensions");
  if (dim > 1 && sizeY > (1 << 22)) throw std::runtime_error("OpenGL error: invalid texture dimensions");
}

TextureBuffer::~TextureBuffer() {}

void TextureBuffer::setFilterMode(FilterMode newMode) {}

void TextureBuffer::resize(unsigned int newLen) { sizeX = newLen; }
void TextureBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}

void TextureBuffer::resize(unsigned int newX, unsigned int newY, unsigned int nSamples) {
  sizeX = newX;
  sizeY = newY;
  multisampleCount = nSamples;
}


RenderBuffer::RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : type(type_), sizeX(sizeX_), sizeY(sizeY_) {
  if (sizeX > (1 << 22) || sizeY > (1 << 22)) throw std::runtime_error("OpenGL error: invalid renderbuffer dimensions");
}

void RenderBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}

void RenderBuffer::resize(unsigned int newX, unsigned int newY, unsigned int nSamples) {
  sizeX = newX;
  sizeY = newY;
  multisampleCount = nSamples;
}

FrameBuffer::FrameBuffer() {}

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

void FrameBuffer::resize(unsigned int newXSize, unsigned int newYSize, unsigned int nSamples) {
  bind();
  for (auto& b : renderBuffersColor) {
    b->resize(newXSize, newYSize, nSamples);
  }
  for (auto& b : renderBuffersDepth) {
    b->resize(newXSize, newYSize, nSamples);
  }
  for (auto& b : textureBuffersColor) {
    b->resize(newXSize, newYSize, nSamples);
  }
  for (auto& b : textureBuffersDepth) {
    b->resize(newXSize, newYSize, nSamples);
  }
  sizeX = newXSize;
  sizeY = newYSize;
}

void FrameBuffer::verifyBufferSizes() {
  for (auto& b : renderBuffersColor) {
    if (b->getSizeX() != getSizeX() || b->getSizeY() != getSizeY())
      throw std::runtime_error("render buffer size does not match framebuffer size");
  }
}

ShaderProgram::ShaderProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm,
                             unsigned int nPatchVertices_)
    : drawMode(dm), nPatchVertices(nPatchVertices_) {

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

    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Tone Mapping")) {
      ImGui::SliderFloat("exposure", &exposure, 0.1, 2.0, "%.3f", 2.);
      ImGui::SliderFloat("white level", &whiteLevel, 0.0, 2.0, "%.3f", 2.);
      ImGui::SliderFloat("gamma", &gamma, 0.5, 3.0, "%.3f", 2.);

      ImGui::TreePop();
    }

    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Anti-Aliasing")) {
      if (ImGui::InputInt("MSAA (fast)", &msaaFactor, 1)) {
        msaaFactor = std::min(msaaFactor, 32);
        msaaFactor = std::max(msaaFactor, 1);
        updateWindowSize(true);
      }
      if (ImGui::InputInt("SSAA (pretty)", &ssaaFactor, 1)) {
        ssaaFactor = std::min(ssaaFactor, 4);
        ssaaFactor = std::max(ssaaFactor, 1);
        updateWindowSize(true);
      }
      ImGui::TreePop();
    }

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

    groundPlane.buildGui();

    ImGui::TreePop();
  }
}

void Engine::setBackgroundColor(glm::vec3 c) { sceneBuffer->clearColor = c; }

void Engine::setBackgroundAlpha(float newAlpha) { sceneBuffer->clearAlpha = newAlpha; }

void Engine::setCurrentViewport(glm::vec4 val) { currViewport = val; }
glm::vec4 Engine::getCurrentViewport() { return currViewport; }
void Engine::setCurrentPixelScaling(float val) { currPixelScale = val; }
float Engine::getCurrentPixelScaling() { return currPixelScale; }

void Engine::bindDisplay() { displayBuffer->bindForRendering(); }


void Engine::clearDisplay() {
  displayBuffer->clear();
  // bindDisplay();
  // glClearColor(1., 1., 1., 0.);
  // glClearDepth(1.);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}


void Engine::clearSceneBuffer() {
  sceneBuffer->clear();
  sceneBufferFinal->clear();
}

void Engine::resizeScreenBuffers() {
  unsigned int width = view::bufferWidth;
  unsigned int height = view::bufferHeight;
  displayBuffer->resize(width, height);
  sceneBuffer->resize(ssaaFactor * width, ssaaFactor * height, msaaFactor);
  sceneBufferFinal->resize(ssaaFactor * width, ssaaFactor * height);
}

void Engine::setScreenBufferViewports() {
  unsigned int xStart = 0;
  unsigned int yStart = 0;
  unsigned int sizeX = view::bufferWidth;
  unsigned int sizeY = view::bufferHeight;

  displayBuffer->setViewport(xStart, yStart, sizeX, sizeY);
  sceneBuffer->setViewport(ssaaFactor * xStart, ssaaFactor * yStart, ssaaFactor * sizeX, ssaaFactor * sizeY);
  sceneBufferFinal->setViewport(ssaaFactor * xStart, ssaaFactor * yStart, ssaaFactor * sizeX, ssaaFactor * sizeY);
}

bool Engine::bindSceneBuffer() {
  setCurrentPixelScaling(ssaaFactor);
  return sceneBuffer->bindForRendering();
}

void Engine::applyLightingTransform(std::shared_ptr<TextureBuffer>& texture) {
  mapLight->setUniform("u_exposure", exposure);
  mapLight->setUniform("u_whiteLevel", whiteLevel);
  mapLight->setUniform("u_gamma", gamma);
  mapLight->setTextureFromBuffer("t_image", texture.get());


  // compute downsampling rate
  glm::vec4 currV = getCurrentViewport();
  float sampleX = texture->getSizeX() / currV[2];
  float sampleY = texture->getSizeY() / currV[3];
  if (sampleX != sampleY) throw std::runtime_error("lighting downsampling should have same aspect");
  int sampleLevel;
  if (sampleX < 1.) {
    sampleLevel = 1;
  } else {
    if (sampleX != static_cast<int>(sampleX))
      throw std::runtime_error("lighting downsampling should have integer ratio");
    sampleLevel = static_cast<int>(sampleX);
    if (sampleLevel > 4) throw std::runtime_error("lighting downsampling only implemented up to 4x");
  }

  mapLight->setUniform("u_downsampleFactor", sampleLevel);
  glm::vec2 texelSize{1. / texture->getSizeX(), 1. / texture->getSizeY()};
  mapLight->setUniform("u_texelSize", texelSize);

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

void Engine::allocateGlobalBuffersAndPrograms() {

  // Note: The display frame buffer should be manually wrapped by child classes

  { // Scene buffer

    // Note that this is basically duplicated in ground_plane.cpp, changes here should probably be reflected there
    // sceneColor = generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
    // sceneDepth = generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);
    sceneColor =
        generateTextureBufferMultisample(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight, msaaFactor);
    sceneDepth =
        generateRenderBufferMultisample(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight, msaaFactor);

    sceneBuffer = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    sceneBuffer->addColorBuffer(sceneColor);
    sceneBuffer->addDepthBuffer(sceneDepth);
    sceneBuffer->setDrawBuffers();

    sceneBuffer->clearColor = glm::vec3{1., 1., 1.};
    sceneBuffer->clearAlpha = 0.0;
  }

  { // "Final" scene buffer (after resolving multisample)
    sceneColorFinal = generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);

    sceneBufferFinal = generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    sceneBufferFinal->addColorBuffer(sceneColorFinal);
    sceneBufferFinal->setDrawBuffers();

    sceneBufferFinal->clearColor = glm::vec3{1., 1., 1.};
    sceneBufferFinal->clearAlpha = 0.0;
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
    renderTexturePlain = generateShaderProgram({TEXTURE_DRAW_VERT_SHADER, PLAIN_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
    renderTexturePlain->setAttribute("a_position", screenTrianglesCoords());

    renderTextureDot3 = generateShaderProgram({TEXTURE_DRAW_VERT_SHADER, DOT3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
    renderTextureDot3->setAttribute("a_position", screenTrianglesCoords());

    renderTextureMap3 = generateShaderProgram({TEXTURE_DRAW_VERT_SHADER, MAP3_TEXTURE_DRAW_FRAG_SHADER}, DrawMode::Triangles);
    renderTextureMap3->setAttribute("a_position", screenTrianglesCoords());

    renderTextureSphereBG = generateShaderProgram({SPHEREBG_DRAW_VERT_SHADER, SPHEREBG_DRAW_FRAG_SHADER}, DrawMode::Triangles);
    renderTextureSphereBG->setAttribute("a_position", distantCubeCoords());
    
    mapLight = generateShaderProgram({TEXTURE_DRAW_VERT_SHADER, MAP_LIGHT_FRAG_SHADER}, DrawMode::Triangles);
    mapLight->setAttribute("a_position", screenTrianglesCoords());
    // clang-format on
  }

  { // Load defaults
    loadDefaultMaterials();
    loadDefaultColorMaps();
  }
}

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
    throw std::runtime_error("unrecognized default material name " + name);
  }
  // clang-format on


  /*
      int w, h, comp;
      unsigned char* image = nullptr;
      image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&data[i][0]), data[i].size(), &w, &h,
     &comp, STBI_rgb); if (image == nullptr) throw std::logic_error("Failed to load material image");

      newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB8, w, h, image);
  newMaterial.textureBuffers[i]->setFilterMode(FilterMode::Linear);
      stbi_image_free(image);
  */

  for (int i = 0; i < 4; i++) {
    int width, height, nComp;
    float* data = stbi_loadf_from_memory(buff[i], buffSize[i], &width, &height, &nComp, 3);
    if (!data) polyscope::error("failed to load material");
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

  throw std::runtime_error("unrecognized material name: " + name);
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

  throw std::runtime_error("unrecognized colormap name: " + name);
  return *colorMaps[0];
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
  } else {
    throw std::runtime_error("unrecognized default colormap " + name);
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
}


void Engine::showTextureInImGuiWindow(std::string windowName, TextureBuffer* buffer) {
  ImGui::Begin(windowName.c_str());

  if (buffer->getDimension() != 2) error("only know how to show 2D textures");

  float w = ImGui::GetWindowWidth();
  float h = w * buffer->getSizeY() / buffer->getSizeX();

  ImGui::Text("Dimensions: %dx%d", buffer->getSizeX(), buffer->getSizeY());
  ImGui::Image(buffer->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

  ImGui::End();
}

void Engine::setImGuiStyle() {

  // Style
  ImGuiStyle* style = &ImGui::GetStyle();
  style->WindowRounding = 1;
  style->FrameRounding = 1;
  style->FramePadding.y = 4;
  style->ScrollbarRounding = 1;
  style->ScrollbarSize = 20;


  // Colors
  ImVec4* colors = style->Colors;
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
  colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.63f, 0.63f, 0.63f, 0.39f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.69f, 0.59f, 0.40f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.41f, 0.64f, 0.53f, 0.69f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.54f, 0.42f, 0.83f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.63f, 0.49f, 0.87f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.27f, 0.54f, 0.42f, 0.83f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.55f, 0.48f, 0.80f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.63f, 0.63f, 0.63f, 0.39f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.80f, 0.62f, 0.40f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
  colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
  colors[ImGuiCol_Button] = ImVec4(0.35f, 0.61f, 0.49f, 0.62f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.71f, 0.57f, 0.79f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.80f, 0.64f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.40f, 0.90f, 0.67f, 0.45f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.90f, 0.69f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.87f, 0.71f, 0.80f);
  colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.70f, 0.66f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.90f, 0.81f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 1.00f, 0.90f, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 1.00f, 0.90f, 0.90f);
  colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
  colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_Tab] = ImVec4(0.27f, 0.54f, 0.42f, 0.83f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.34f, 0.68f, 0.53f, 0.83f);
  colors[ImGuiCol_TabActive] = ImVec4(0.38f, 0.76f, 0.58f, 0.83f);
}

ImFontAtlas* Engine::getImGuiGlobalFontAtlas() { return globalFontAtlas; }

} // namespace render
} // namespace polyscope

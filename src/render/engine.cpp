// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/engine.h"

#include "polyscope/polyscope.h"

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


RenderBuffer::RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : type(type_), sizeX(sizeX_), sizeY(sizeY_) {
  if (sizeX > (1 << 22) || sizeY > (1 << 22)) throw std::runtime_error("OpenGL error: invalid renderbuffer dimensions");
}

void RenderBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}

FrameBuffer::FrameBuffer() {}

void FrameBuffer::setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY) {
  viewportX = startX;
  viewportY = startY;
  viewportSizeX = sizeX;
  viewportSizeY = sizeY;
  viewportSet = true;
}

void FrameBuffer::resizeBuffers(unsigned int newXSize, unsigned int newYSize) {
  for (auto& b : renderBuffers) {
    b->resize(newXSize, newYSize);
  }
  for (auto& b : textureBuffers) {
    b->resize(newXSize, newYSize);
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
  // == Display
  ImGui::PushItemWidth(120);
  ImGui::Text("Background");
  ImGui::SameLine();
  static std::string displayBackgroundName = "None";
  if (ImGui::BeginCombo("##Background", displayBackgroundName.c_str())) {
    if (ImGui::Selectable("Albedo", background == BackgroundView::None)) {
      background = BackgroundView::None;
      ImGui::SetItemDefaultFocus();
      displayBackgroundName = "Albedo";
    }
    if (ImGui::Selectable("Enviornment", background == BackgroundView::Env)) {
      background = BackgroundView::Env;
      ImGui::SetItemDefaultFocus();
      displayBackgroundName = "Environment";
    }
    if (ImGui::Selectable("Environment (diffuse)", background == BackgroundView::EnvDiffuse)) {
      background = BackgroundView::EnvDiffuse;
      ImGui::SetItemDefaultFocus();
      displayBackgroundName = "Environment (diffuse)";
    }
    if (ImGui::Selectable("Environment (specular)", background == BackgroundView::EnvSpecular)) {
      background = BackgroundView::EnvSpecular;
      ImGui::SetItemDefaultFocus();
      displayBackgroundName = "Environment (specular)";
    }
    ImGui::EndCombo();
  }

  ImGui::SliderFloat("exposure", &exposure, 0.1, 5.0, "%.3f", 2.);
  ImGui::SliderFloat("light stength", &lightStrength, 0.0, 5.0, "%.3f", 2.);
  ImGui::SliderFloat("ambient strength", &ambientStrength, 0.0, 1.0, "%.3f", 2.);

  groundPlane.buildGui();
}

void Engine::setBackgroundColor(glm::vec3) {
  // TODO
}

void Engine::setBackgroundAlpha(float newAlpha) {
  // TODO
}

// bool Engine::bindSceneBuffer() { return sceneBuffer->bindForRendering(); }

void Engine::clearSceneBuffer() { sceneBuffer->clear(); }

void Engine::resizeSceneBuffer(int width, int height) { sceneBuffer->resizeBuffers(width, height); }

void Engine::setSceneBufferViewport(int xStart, int yStart, int sizeX, int sizeY) {
  sceneBuffer->setViewport(xStart, yStart, sizeX, sizeY);
}

bool Engine::bindSceneBuffer() { return sceneBuffer->bindForRendering(); }

void Engine::lightSceneBuffer() {
  mapLight->setUniform("u_exposure", exposure);
  mapLight->setTextureFromBuffer("t_image", sceneColor.get());
  mapLight->draw();
}

void Engine::setGlobalLightingParameters(ShaderProgram& program) {
  program.setUniform("u_ambientStrength", ambientStrength);
  program.setUniform("u_lightStrength", lightStrength);
  if (program.hasTexture("t_envDiffuse")) {
    program.setTextureFromBuffer("t_envDiffuse", envMapDiffuse.get());
    program.setTextureFromBuffer("t_envSpecular", envMapSpecular.get());
    program.setTextureFromBuffer("t_specularPrecomp", specularSplitPrecomp.get());
  }
}

void Engine::renderBackground() {
  switch (background) {
  case BackgroundView::None:
    break;
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
  case BackgroundView::EnvDiffuse: {
    glm::mat4 V = view::getCameraViewMatrix();
    glm::mat4 P = view::getCameraPerspectiveMatrix();
    renderTextureSphereBG->setUniform("u_viewMatrix", glm::value_ptr(V));
    renderTextureSphereBG->setUniform("u_projMatrix", glm::value_ptr(P));
    renderTextureSphereBG->setTextureFromBuffer("t_image", envMapDiffuse.get());
    setDepthMode(DepthMode::LEqualReadOnly);
    renderTextureSphereBG->draw();
  } break;
  case BackgroundView::EnvSpecular: {
    glm::mat4 V = view::getCameraViewMatrix();
    glm::mat4 P = view::getCameraPerspectiveMatrix();
    renderTextureSphereBG->setUniform("u_viewMatrix", glm::value_ptr(V));
    renderTextureSphereBG->setUniform("u_projMatrix", glm::value_ptr(P));
    renderTextureSphereBG->setTextureFromBuffer("t_image", specularSplitPrecomp.get());
    setDepthMode(DepthMode::LEqualReadOnly);
    renderTextureSphereBG->draw();
  } break;
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

void Engine::loadEnvironmentMap(std::string mapFilename, std::string diffuseFilename) {

  stbi_set_flip_vertically_on_load(true);

  int width, height, nrComponents;
  float* data = stbi_loadf(mapFilename.c_str(), &width, &height, &nrComponents, 0);
  unsigned int hdrTexture;
  if (!data) {
    error("failed to load environment map at " + mapFilename);
    return;
  }

  // Load the texture
  envMapOrig = generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
  envMapOrig->setFilterMode(FilterMode::Linear);

  // TODO generate these
  if (diffuseFilename == "") {
    envMapDiffuse = generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
  } else {
    int width, height, nrComponents;
    float* data = stbi_loadf(diffuseFilename.c_str(), &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (!data) {
      error("failed to load environment map at " + diffuseFilename);
      return;
    }
    envMapDiffuse = generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
  }
  envMapDiffuse->setFilterMode(FilterMode::Linear);

  envMapSpecular = generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
  envMapSpecular->setFilterMode(FilterMode::Linear);

  stbi_image_free(data);
}

} // namespace render
} // namespace polyscope

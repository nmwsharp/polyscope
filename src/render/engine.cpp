// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/engine.h"

#include "polyscope/polyscope.h"
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
    if (ImGui::Selectable("None", background == BackgroundView::None)) {
      background = BackgroundView::None;
      ImGui::SetItemDefaultFocus();
      displayBackgroundName = "None";
    }
    ImGui::EndCombo();
  }

  ImGui::SliderFloat("exposure", &exposure, 0.1, 5.0, "%.3f", 2.);
  ImGui::SliderFloat("white level", &whiteLevel, 0.0, 5.0, "%.3f", 2.);
  ImGui::SliderFloat("gamma", &gamma, 0.1, 5.0, "%.3f", 2.);

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
  mapLight->setUniform("u_whiteLevel", whiteLevel);
  mapLight->setUniform("u_gamma", gamma);
  mapLight->setTextureFromBuffer("t_image", sceneColor.get());
  mapLight->draw();
}

void Engine::setMaterial(ShaderProgram& program, Material mat) {
  BasisMaterial& material = materialCache[mat];
  program.setTextureFromBuffer("t_mat_r", material.textureBuffers[0].get());
  program.setTextureFromBuffer("t_mat_g", material.textureBuffers[1].get());
  program.setTextureFromBuffer("t_mat_b", material.textureBuffers[2].get());
  program.setTextureFromBuffer("t_mat_k", material.textureBuffers[3].get());
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

  { // Scene buffer

    // Note that this is basically duplicated in ground_plane.cpp, changes here should probably be reflected there
    sceneColor = generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
    sceneDepth = generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);

    sceneBuffer = generateFrameBuffer();
    sceneBuffer->addColorBuffer(sceneColor);
    sceneBuffer->addDepthBuffer(sceneDepth);
    sceneBuffer->setDrawBuffers();

    sceneBuffer->clearColor = glm::vec3{1., 1., 1.};
    sceneBuffer->clearAlpha = 0.0;
  }

  { // Pick buffer
    pickColorBuffer = generateRenderBuffer(RenderBufferType::Float4, view::bufferWidth, view::bufferHeight);
    pickDepthBuffer = generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);

    pickFramebuffer = generateFrameBuffer();
    pickFramebuffer->addColorBuffer(pickColorBuffer);
    pickFramebuffer->addDepthBuffer(pickDepthBuffer);
    pickFramebuffer->setDrawBuffers();
  }

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

	{ // Load default materials
		materialCache = loadDefaultMaterials();	
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


} // namespace render
} // namespace polyscope

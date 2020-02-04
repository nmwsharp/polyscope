// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/engine.h"

#include "polyscope/polyscope.h"

#include "imgui.h"


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
    ImGui::Text("Show Buffer");
    ImGui::SameLine();
    static std::string displayResultName = "Final";
    if (ImGui::BeginCombo("##Up Direction", displayResultName.c_str())) {
      if (ImGui::Selectable("Albedo", resultToDisplay == RenderResult::Albedo)) {
        resultToDisplay = RenderResult::Albedo;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Albedo";
      }
      if (ImGui::Selectable("Roughness", resultToDisplay == RenderResult::Roughness)) {
        resultToDisplay = RenderResult::Roughness;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Roughness";
      }
      if (ImGui::Selectable("Metallic", resultToDisplay == RenderResult::Metallic)) {
        resultToDisplay = RenderResult::Metallic;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Metallic";
      }
      if (ImGui::Selectable("Depth", resultToDisplay == RenderResult::Depth)) {
        resultToDisplay = RenderResult::Depth;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Depth";
      }
      if (ImGui::Selectable("Normal", resultToDisplay == RenderResult::Normal)) {
        resultToDisplay = RenderResult::Normal;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Normal";
      }
      if (ImGui::Selectable("Position", resultToDisplay == RenderResult::Position)) {
        resultToDisplay = RenderResult::Position;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Position";
      }
      if (ImGui::Selectable("Final", resultToDisplay == RenderResult::Final)) {
        resultToDisplay = RenderResult::Final;
        ImGui::SetItemDefaultFocus();
        displayResultName = "Final";
      }
      ImGui::EndCombo();
    }
}

void Engine::setBackgroundColor(glm::vec3) {
  // TODO
}

void Engine::setBackgroundAlpha(float newAlpha) {
  // TODO
}

// bool Engine::bindGBuffer() { return GBuffer->bindForRendering(); }

void Engine::clearGBuffer() { 
  GBuffer->clear(); 
}

void Engine::resizeGBuffer(int width, int height) { GBuffer->resizeBuffers(width, height); }

void Engine::setGBufferViewport(int xStart, int yStart, int sizeX, int sizeY) {
  GBuffer->setViewport(xStart, yStart, sizeX, sizeY);
}

bool Engine::bindGBuffer() { return GBuffer->bindForRendering(); }

void Engine::copyGBufferToDisplay() {

  // Bind to the view framebuffer
  bindDisplay();

  switch (resultToDisplay) {
  case RenderResult::Albedo:
    renderTexturePlain->setTextureFromBuffer("t_image", gAlbedo.get());
    renderTexturePlain->draw();
    break;
  case RenderResult::Roughness:
    renderTextureDot3->setTextureFromBuffer("t_image", gMaterial.get());
    renderTextureDot3->setUniform("u_mapDot", glm::vec3{1., 0., 0.});
    renderTextureDot3->draw();
    break;
  case RenderResult::Metallic:
    renderTextureDot3->setTextureFromBuffer("t_image", gMaterial.get());
    renderTextureDot3->setUniform("u_mapDot", glm::vec3{0., 1., 0.});
    renderTextureDot3->draw();
    break;
  case RenderResult::Depth:
    // TODO
    break;
  case RenderResult::Normal:
    renderTextureMap3->setTextureFromBuffer("t_image", gViewNormal.get());
    renderTextureMap3->setUniform("u_shift", glm::vec3(-0.5f));
    renderTextureMap3->setUniform("u_scale", glm::vec3(2.f));
    renderTextureMap3->draw();
    //renderTextureDot3->setTextureFromBuffer("t_image", gViewNormal.get());
    //renderTextureDot3->setUniform("u_mapDot", glm::vec3{1., 0., 0.});
    //renderTextureDot3->draw();
    break;
  case RenderResult::Position:
    renderTextureMap3->setTextureFromBuffer("t_image", gViewPosition.get());
    renderTextureMap3->setUniform("u_shift", glm::vec3(0.f));
    renderTextureMap3->setUniform("u_scale", glm::vec3(1.0 / state::lengthScale));
    renderTextureMap3->draw();
    break;
  case RenderResult::Final:
    renderTexturePlain->setTextureFromBuffer("t_image", gFinal.get());
    renderTexturePlain->draw();
    break;
  }
}


std::vector<glm::vec3> Engine::screenTrianglesCoords() {
  std::vector<glm::vec3> coords = {{-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f},
                                   {-1.0f, 1.0f, 0.0f},  {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};
  return coords;
}

} // namespace render
} // namespace polyscope

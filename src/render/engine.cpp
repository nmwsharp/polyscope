// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/engine.h"


namespace polyscope {
namespace render {

TextureBuffer::TextureBuffer(int dim_, TextureFormat format_, unsigned int sizeX_, unsigned int sizeY_)
    : dim(dim_), format(format_), sizeX(sizeX_), sizeY(sizeY_) {
  if (sizeX > (1 << 22)) throw std::runtime_error("OpenGL error: invalid texture dimensions");
  if (dim > 1 && sizeY > (1 << 22)) throw std::runtime_error("OpenGL error: invalid texture dimensions");
}

TextureBuffer::~TextureBuffer() {}

void TextureBuffer::setFilterMode(FilterMode newMode) {}
  
void TextureBuffer::resize(unsigned int newLen) {
  sizeX = newLen;
}
void TextureBuffer::resize(unsigned int newX, unsigned int newY) {
  sizeX = newX;
  sizeY = newY;
}


RenderBuffer::RenderBuffer(RenderBufferType type_, unsigned int sizeX_, unsigned int sizeY_)
    : type(type_), sizeX(sizeX_), sizeY(sizeY_) {
  if (sizeX > (1 << 22) || sizeY > (1 << 22)) throw std::runtime_error("OpenGL error: invalid renderbuffer dimensions");
}

FrameBuffer::FrameBuffer() {}

void FrameBuffer::setViewport(int startX, int startY, unsigned int sizeX, unsigned int sizeY) {
  viewportX = startX;
  viewportY = startY;
  viewportSizeX = sizeX;
  viewportSizeY = sizeY;
  viewportSet = true;
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

void Engine::setBackgroundColor(glm::vec3) {
  // TODO
}

void Engine::setBackgroundAlpha(float newAlpha) {
  // TODO
}

//bool Engine::bindGBuffer() { return GBuffer->bindForRendering(); }

} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <vector>

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

// ==========================================================
// === Attribute buffers
// ==========================================================

// Allocate a buffer to hold a given template type
// (use std::array<T>s to get arraycount repeated attributes)
template <typename T>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer(Engine* engine);

// Get a single data value from a buffer of a templated type
// (use std::array<T>s to get arraycount repeated attributes)
template <typename T>
T getAttributeBufferData(AttributeBuffer& buff, size_t ind);

// Get a range of data values from a buffer of a templated type
// (use std::array<T>s to get arraycount repeated attributes)
template <typename T>
std::vector<T> getAttributeBufferDataRange(AttributeBuffer& buff, size_t ind, size_t count);


// ==========================================================
// === Texture buffers
// ==========================================================

// Allocate a texture buffer to hold a given template type
// (use std::array<T>s to get arraycount repeated attributes)
template <typename T, DeviceBufferType D>
std::shared_ptr<TextureBuffer> generateTextureBuffer(Engine* engine) {
  exception("bad call"); // default implementation, should be specialized to use
  return nullptr;
}
// this one dispatches dynamically on D
template <typename T>
std::shared_ptr<TextureBuffer> generateTextureBuffer(DeviceBufferType D, Engine* engine);

// Get a single data value from a texturebuffer of a templated type
// (use std::array<T>s to get arraycount repeated attributes)
// openGL doesn't support this anyway...
// template <typename T, DeviceBufferType D>
// T getTextureBufferData(TextureBuffer& buff, size_t indX, size_t indY = 0, size_t indZ = 0);

// Get a range of data values from a buffer of a templated type
// (use std::array<T>s to get arraycount repeated attributes)
template <typename T, DeviceBufferType D>
std::vector<T> getTextureBufferData(TextureBuffer& buff) {
  // TODO write these specializations
  exception("bad call"); // default implementation, should be specialized to use
  return std::vector<T>();
}


} // namespace render
} // namespace polyscope

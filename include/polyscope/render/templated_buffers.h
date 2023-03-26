// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <vector>

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

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

} // namespace render
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

// Allocate a buffer to hold a given template type
template <typename T>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer(Engine* engine, int arrayCount = 1);

// Get a single data value from a buffer of a templated type
template <typename T>
T getAttributeBufferData(AttributeBuffer& buff, size_t ind);

// Get a range of data values from a buffer of a templated type
template <typename T>
std::vector<T> getAttributeBufferDataRange(AttributeBuffer& buff, size_t ind, size_t count);

} // namespace render
} // namespace polyscope

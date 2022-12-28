// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

template <typename T>
std::shared_ptr<AttributeBuffer> generateAttributeBuffer(Engine* engine, int arrayCount = 1);

} // namespace render
} // namespace polyscope

#include "polyscope/render/templated_buffers.ipp"

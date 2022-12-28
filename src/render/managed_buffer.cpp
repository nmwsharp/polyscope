// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/render/managed_buffer.h"

#include "polyscope/messages.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/templated_buffers.h"

namespace polyscope {
namespace render {

template <typename T>
ManagedBuffer<T>::ManagedBuffer(const std::string& name_, std::vector<T>& data_, ManagedBuffer<size_t>* indices_)
    : name(name_), data(data_), dataGetsComputed(false), indices(indices_) {}

template <typename T>
ManagedBuffer<T>::ManagedBuffer(const std::string& name_, std::vector<T>& data_, std::function<void()> computeFunc_,
                                ManagedBuffer<size_t>* indices_)
    : name(name_), data(data_), dataGetsComputed(true), computeFunc(computeFunc_), indices(indices_) {}


template <typename T>
void ManagedBuffer<T>::ensureHostBufferPopulated() {
  // TODO
}

template <typename T>
void ManagedBuffer<T>::markHostBufferUpdated() {

  if (indices == nullptr) {
    // Non-indexed case.
    // There is only possibly one buffer; update it if it exists
    // (even if the renderBuffer ptr is non-null, it points to the same underlying buffer)
    if (drawBuffer) {
    }
  } else {
    // Indexed case.

    if (renderBuffer) {
      // If the render buffer is active, immediately copy the data there and trigger a new re-indexing render pass

      // TODO
    } else if (drawBuffer) {
      // If the draw buffer is active but the render buffer is not, copy the new data to it with the permutation

    } else {
      // Otherwise, the data has not be pushed to any device buffer yet. Do nothing.
    }
  }
}

template <typename T>
T ManagedBuffer<T>::getValue(size_t ind) {

  switch (currentCanonicalDataSource()) {
  case CanonicalDataSource::HostData:
    if (ind >= data.size())
      error("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::NeedsCompute:
    computeFunc();
    if (ind >= data.size())
      error("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::RenderBuffer:
    // TODO
    break;
  };
}

template <typename T>
void ManagedBuffer<T>::recomputeIfPopulated() {
  if (!dataGetsComputed) {
    error("called recomputeIfPopulated() on buffer which does not get computed");
  }

  // if not populated, quick out
  if (currentCanonicalDataSource() == CanonicalDataSource::NeedsCompute) {
    return;
  }

  invalidateHostBuffer();
  computeFunc();
  markHostBufferUpdated();
}

// template <typename T>
// void ManagedBuffer<T>::ensureRenderBufferPopulated() {}

template <typename T>
std::shared_ptr<render::AttributeBuffer> ManagedBuffer<T>::getRenderBuffer() {
  if (renderBuffer) return renderBuffer; // if it has already been set, just return the pointer

  // TODO ensure that if the renderBuffer exists, the drawBuffer does too

  // TODO

  return renderBuffer;
}

template <typename T>
void ManagedBuffer<T>::markRenderBufferUpdated() {
  invalidateHostBuffer();
  requestRedraw();
}

template <typename T>
std::shared_ptr<render::AttributeBuffer> ManagedBuffer<T>::getDrawBuffer() {
  if (drawBuffer) return drawBuffer; // if it has already been set, just return the pointer

  // Allocate it
  drawBuffer = generateAttributeBuffer<T>(render::engine);

  // Fill with initial data
  ensureHostBufferPopulated();

  // NOTE: we should not need to worry about the case where the renderBuffer exists and data exists there but not on the
  // host---in that case, the draw buffer must already exist/be filled and thus have been quick-out returned above.
  
  if(indices) {
    // Expand out the array with the gather indexing applied
    indices->ensureHostBufferPopulated();
    std::vector<T> expandData = applyPermutation(data, indices->data);

    drawBuffer->setData(expandData);
  } else {
    // Plain old ordinary copy
    drawBuffer->setData(data);
  }

  return drawBuffer;
}

template <typename T>
void ManagedBuffer<T>::invalidateHostBuffer() {
  data.clear();
}

template <typename T>
typename ManagedBuffer<T>::CanonicalDataSource ManagedBuffer<T>::currentCanonicalDataSource() {

  // Always prefer the host data if it is up to date
  if (!data.empty()) {
    return CanonicalDataSource::HostData;
  }

  // Check if either the render or draw buffer contains the canonical data
  if (renderBuffer) {
    return CanonicalDataSource::RenderBuffer;
  }

  if (dataGetsComputed) {
    return CanonicalDataSource::NeedsCompute;
  }

  // error! should always be one of the above
  error("ManagedBuffer " + name + " is in an invalid state");
  return CanonicalDataSource::HostData; // dummy return
}

// === Explicit template instantiation for the supported types

template class ManagedBuffer<double>;


} // namespace render
} // namespace polyscope

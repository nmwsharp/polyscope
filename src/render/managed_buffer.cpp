// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

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

  switch (currentCanonicalDataSource()) {
  case CanonicalDataSource::HostData:
    // good to go, nothing needs to be done
    break;

  case CanonicalDataSource::NeedsCompute:

    // compute it
    computeFunc();

    break;

  case CanonicalDataSource::RenderBuffer:

    // sanity check
    if (!renderBuffer) terminatingError("render buffer should be allocated but isn't");

    // copy the data back from the renderBuffer
    data = getAttributeBufferDataRange<T>(*renderBuffer, 0, renderBuffer->getDataSize());

    break;
  };
}

template <typename T>
void ManagedBuffer<T>::markHostBufferUpdated() {

  // If the data is stored in any device-side buffers, update it as needed

  if (indices == nullptr) {
    // Non-indexed case.
    // There is only possibly one buffer; update it if it exists
    // (even if the renderBuffer ptr is non-null, it points to the same underlying buffer)
    if (drawBuffer) {
      drawBuffer->setData(data);
    }
  } else {
    // Indexed case.

    if (renderBuffer) {
      // If the render buffer is active, immediately copy the data there and trigger a new re-indexing render pass

      renderBuffer->setData(data);
      invokeBufferIndexCopyProgram();

    } else if (drawBuffer) {
      // If the draw buffer is active but the render buffer is not, copy the new data to it with the permutation

      copyHostDataToDrawBuffer();

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
      terminatingError("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::NeedsCompute:
    computeFunc();
    if (ind >= data.size())
      terminatingError("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::RenderBuffer:
    if (static_cast<int64_t>(ind) >= renderBuffer->getDataSize())
      terminatingError("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    T val = getAttributeBufferData<T>(*renderBuffer, ind);
    return val;
    break;
  };

  return T(); // dummy return
}

template <typename T>
void ManagedBuffer<T>::recomputeIfPopulated() {
  if (!dataGetsComputed) { // sanity check
    terminatingError("called recomputeIfPopulated() on buffer which does not get computed");
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

  // Ensure that if the renderBuffer exists, the drawBuffer does too.
  // We ignore the return, but after calling, the drawBuffer member must be populated.
  getDrawBuffer();

  // TODO
  if (indices == nullptr) {
    // If there is no indexing to apply, then we don't need a separate buffer. The
    // drawBuffer is the renderBuffer and we can just directly write to it.
    renderBuffer = drawBuffer;
  } else {
    // Create the render buffer
    renderBuffer = generateAttributeBuffer<T>(render::engine);

    // Initially populate the render buffer with data (in its canonical form)
    ensureHostBufferPopulated();
    renderBuffer->setData(data);

    // Create a program which will apply indexing and copy updated renderBuffer data to the drawBuffer
    ensureHaveBufferIndexCopyProgram();
  }

  return renderBuffer;
}

template <typename T>
void ManagedBuffer<T>::markRenderBufferUpdated() {
  invalidateHostBuffer();
  if (indices) {
    invokeBufferIndexCopyProgram();
  }
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
  copyHostDataToDrawBuffer();

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
  terminatingError("ManagedBuffer " + name + " is in an invalid state");
  return CanonicalDataSource::HostData; // dummy return
}


template <typename T>
void ManagedBuffer<T>::ensureHaveBufferIndexCopyProgram() {
  if (bufferIndexCopyProgram) return;

  // sanity check
  if (!drawBuffer || !renderBuffer)
    terminatingError("ManagedBuffer " + name + " asked to copy indices, but has no buffers");

  // TODO allocate the transform feedback program
}

template <typename T>
void ManagedBuffer<T>::invokeBufferIndexCopyProgram() {
  ensureHaveBufferIndexCopyProgram();
  bufferIndexCopyProgram->draw();
}

template <typename T>
void ManagedBuffer<T>::copyHostDataToDrawBuffer() {

  // sanity check
  if (!drawBuffer) terminatingError("ManagedBuffer " + name + " asked to copy to drawBuffer, but it is not allocated");

  if (indices) {
    // Expand out the array with the gather indexing applied
    indices->ensureHostBufferPopulated();
    std::vector<T> expandData = applyPermutation(data, indices->data);
    drawBuffer->setData(expandData);
  } else {
    // Plain old ordinary copy
    drawBuffer->setData(data);
  }
}

// === Explicit template instantiation for the supported types

template class ManagedBuffer<double>;


} // namespace render
} // namespace polyscope

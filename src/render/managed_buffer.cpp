// Copyright 2018-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include <vector>

#include "polyscope/render/managed_buffer.h"

#include "polyscope/check_invalid_values.h"
#include "polyscope/internal.h"
#include "polyscope/messages.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/templated_buffers.h"

namespace polyscope {
namespace render {

// === ManagedBufferBase implementations ===

ManagedBufferBase::ManagedBufferBase(ManagedBufferRegistry* registry_, const std::string& name_, bool dataGetsComputed_,
                                     bool hostBufferValid_)
    : name(name_), uniqueID(internal::getNextUniqueID()), registry(registry_), dataGetsComputed(dataGetsComputed_),
      hostBufferValid(hostBufferValid_), deviceBufferValid(false) {}

void ManagedBufferBase::invalidateHostBuffer() {
  hostBufferValid = false;
  // Note: data.clear() is handled by ManagedBuffer<T> which overrides this conceptually by
  // calling the base version and then clearing its own data vector.
}

bool ManagedBufferBase::deviceBufferTypeIsTexture() const {
  return ((deviceBufferType == DeviceBufferType::Texture1d) || (deviceBufferType == DeviceBufferType::Texture2d) ||
          (deviceBufferType == DeviceBufferType::Texture3d));
}

void ManagedBufferBase::checkDeviceBufferTypeIs(DeviceBufferType targetType) const {
  if (targetType != deviceBufferType) {
    exception("ManagedBuffer " + name + " has wrong type for this operation. Expected " +
              deviceBufferTypeName(targetType) + " but is " + deviceBufferTypeName(deviceBufferType));
  }
}

void ManagedBufferBase::checkDeviceBufferTypeIsTexture() const {
  if (!deviceBufferTypeIsTexture()) {
    exception("ManagedBuffer " + name + " has wrong type for this operation. Expected a Texture1d/2d/3d but is " +
              deviceBufferTypeName(deviceBufferType));
  }
}


// === ManagedBuffer<T> implementations ===

template <typename T>
ManagedBuffer<T>::ManagedBuffer(ManagedBufferRegistry* registry_, const std::string& name_)
    : ManagedBufferBase(registry_, name_, /*dataGetsComputed=*/false, /*hostBufferValid=*/true) {

  managedCapacity = 0;
  currentSize = 0;

  if (registry) {
    registry->addManagedBuffer<T>(this);
  }
}

template <typename T>
ManagedBuffer<T>::ManagedBuffer(ManagedBufferRegistry* registry_, const std::string& name_, std::vector<T> data_)
    : ManagedBufferBase(registry_, name_, /*dataGetsComputed=*/false, /*hostBufferValid=*/true),
      data(std::move(data_)) {

  managedCapacity = data.size();
  currentSize = data.size();

  if (registry) {
    registry->addManagedBuffer<T>(this);
  }
}


template <typename T>
ManagedBuffer<T>::ManagedBuffer(ManagedBufferRegistry* registry_, const std::string& name_,
                                std::function<void()> computeFunc_)
    : ManagedBufferBase(registry_, name_, /*dataGetsComputed=*/true, /*hostBufferValid=*/false) {

  computeFunc = computeFunc_;
  managedCapacity = 0;
  currentSize = 0;

  if (registry) {
    registry->addManagedBuffer<T>(this);
  }
}

template <typename T>
ManagedBuffer<T>::~ManagedBuffer() {
  if (registry) {
    registry->removeManagedBuffer<T>(this);
  }
}

template <typename T>
void ManagedBuffer<T>::checkInvalidValues() {
  // Only check the logically-valid elements; slack in [currentSize, managedCapacity) is uninitialized.
  polyscope::checkInvalidValues(name, std::vector<T>(data.begin(), data.begin() + currentSize));
}

template <typename T>
void ManagedBuffer<T>::setAsType(DeviceBufferType type) {
  if (deviceBufferType != DeviceBufferType::Attribute)
    exception("ManagedBuffer " + name + " setAsType(): type has already been set");
  deviceBufferType = type;
}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_) {
  checkDeviceBufferTypeIs(DeviceBufferType::Texture1d);
  resize(static_cast<size_t>(sizeX_));
}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_, uint32_t sizeY_) {
  checkDeviceBufferTypeIs(DeviceBufferType::Texture2d);
  if (sizeX_ == sizeX && sizeY_ == sizeY) return; // no-op
  if (managedCapacity > 0) {
    // Resize: copy device-side data back to host before touching dimensions or the GPU buffer.
    ensureHostBufferPopulated();
    managedCapacity = static_cast<size_t>(sizeX_) * sizeY_;
    currentSize = managedCapacity;
    data.resize(managedCapacity);
    if (renderTextureBuffer) renderTextureBuffer->resize(sizeX_, sizeY_);
    hostBufferValid = true;
    deviceBufferValid = false;
  }
  sizeX = sizeX_;
  sizeY = sizeY_;
}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_, uint32_t sizeY_, uint32_t sizeZ_) {
  checkDeviceBufferTypeIs(DeviceBufferType::Texture3d);
  if (sizeX_ == sizeX && sizeY_ == sizeY && sizeZ_ == sizeZ) return; // no-op
  if (managedCapacity > 0) {
    // Resize: copy device-side data back to host before touching dimensions or the GPU buffer.
    ensureHostBufferPopulated();
    managedCapacity = static_cast<size_t>(sizeX_) * sizeY_ * sizeZ_;
    currentSize = managedCapacity;
    data.resize(managedCapacity);
    if (renderTextureBuffer) renderTextureBuffer->resize(sizeX_, sizeY_, sizeZ_);
    hostBufferValid = true;
    deviceBufferValid = false;
  }
  sizeX = sizeX_;
  sizeY = sizeY_;
  sizeZ = sizeZ_;
}

template <typename T>
std::array<uint32_t, 3> ManagedBuffer<T>::getTextureSize() const {
  if (deviceBufferType == DeviceBufferType::Attribute) exception("managed buffer is not a texture");
  return std::array<uint32_t, 3>{sizeX, sizeY, sizeZ};
}

template <typename T>
size_t ManagedBuffer<T>::capacity() const {
  return managedCapacity;
}

template <typename T>
bool ManagedBuffer<T>::resize(size_t newSize) {
  if (deviceBufferType == DeviceBufferType::Texture2d || deviceBufferType == DeviceBufferType::Texture3d)
    exception("resize() is not valid for 2D/3D texture buffers; use setTextureSize() instead");

  if (newSize > managedCapacity) {
    // Copy device-side data back to host BEFORE modifying data or invalidating the GPU buffer.
    // ensureHostBufferPopulated() reads from renderAttributeBuffer into data; if we resize data
    // first it would overwrite the resize with the old GPU contents.
    // Only copy existing data to host if there is data to preserve.
    if (isInNeedsComputeState()) {
      // If state is NeedsCompute, there is no existing data — calling ensureHostBufferPopulated()
      // would recursively invoke the compute function that is currently running.
    } else {
      // Common case
      ensureHostBufferPopulated();
    }

    // Reallocation needed: use amortized doubling.
    // data is always kept at data.size() == managedCapacity (the invariant), so we resize (not reserve).
    size_t newCapacity = std::max(newSize, 2 * managedCapacity);
    data.resize(newCapacity);
    managedCapacity = newCapacity;
    currentSize = newSize;

    hostBufferValid = true;
    deviceBufferValid = false;

    if (deviceBufferType == DeviceBufferType::Texture1d) {
      sizeX = static_cast<uint32_t>(newSize);
    }

    return true;
  } else {
    // No reallocation: just update currentSize.
    // data is already at managedCapacity; restore it if it was cleared by invalidateHostBuffer().
    if (data.size() != managedCapacity) data.resize(managedCapacity);
    currentSize = newSize;
    hostBufferValid = true;
    deviceBufferValid = false;

    if (deviceBufferType == DeviceBufferType::Texture1d) {
      sizeX = static_cast<uint32_t>(newSize);
    }

    return false;
  }
}

template <typename T>
void ManagedBuffer<T>::setCapacity(size_t newCapacity) {
  if (deviceBufferType == DeviceBufferType::Texture2d || deviceBufferType == DeviceBufferType::Texture3d)
    exception("setCapacity() is not valid for 2D/3D texture buffers");

  if (newCapacity < currentSize)
    exception("setCapacity() cannot set capacity below current size (" + std::to_string(currentSize) + ")");

  if (newCapacity == managedCapacity) return; // no-op

  // Before invalidating the GPU buffer, copy any device-side changes back to the host.
  // Skip if state is NeedsCompute — there's no existing data to preserve.
  if (!isInNeedsComputeState()) {
    ensureHostBufferPopulated();
  }

  // Resize data to the new capacity (maintaining the invariant data.size() == managedCapacity).
  data.resize(newCapacity);
  managedCapacity = newCapacity;

  hostBufferValid = true;
  deviceBufferValid = false;
}


template <typename T>
void ManagedBuffer<T>::ensureHostBufferPopulated() {

  if (hostBufferValid) {
    // good to go, nothing needs to be done
    return;
  }

  if (!hostBufferValid && deviceBufferValid) {
    // RenderBuffer case: copy data back from device to host
    if (deviceBufferTypeIsTexture()) {
      if (!renderTextureBuffer) exception("render buffer should be allocated but isn't");

      // copy the data back from the renderBuffer
      // TODO not implemented yet
      exception("copy-back from texture not implemented yet");
    } else {
      // sanity check
      if (!renderAttributeBuffer) exception("render buffer should be allocated but isn't");

      // copy the data back from the renderBuffer
      // The GPU buffer is always managedCapacity-sized; only currentSize elements are valid.
      // currentSize is already correct (set at the last resize/setDataHost); don't overwrite it.
      std::vector<T> readback = getAttributeBufferDataRange<T>(*renderAttributeBuffer, 0, currentSize);
      data.resize(managedCapacity); // maintain invariant: data.size() == managedCapacity
      std::copy(readback.begin(), readback.end(), data.begin());
      hostBufferValid = true;
    }
    return;
  }

  if (isInNeedsComputeState()) {
    // NeedsCompute case: run the compute function
    computeFunc();
    // Note: computeFunc is expected to call resize()+setHostValue() or setDataHost(), which sets hostBufferValid=true
    return;
  }

  // error! should always be one of the above
  exception("ManagedBuffer " + name + " does not have data in either host or device buffers, nor a compute function.");
}


template <typename T>
void ManagedBuffer<T>::setDataHost(const std::vector<T>& newData) {
  if (newData.size() > managedCapacity)
    exception("ManagedBuffer " + name + " setDataHost() called with data that exceeds capacity (" +
              std::to_string(newData.size()) + " > " + std::to_string(managedCapacity) +
              "). Call resize() or setCapacity() first.");
  // Restore the invariant (data may have been cleared by invalidateHostBuffer()).
  if (data.size() != managedCapacity) data.resize(managedCapacity);
  std::copy(newData.begin(), newData.end(), data.begin());
  currentSize = newData.size();
  hostBufferValid = true;
  deviceBufferValid = false;
  invalidateIndexedViews();
  requestRedraw();
}

template <typename T>
std::vector<T> ManagedBuffer<T>::getDataCopy() {
  ensureHostBufferPopulated();
  // Return only the logically-valid elements; data may have capacity slack past currentSize.
  return std::vector<T>(data.begin(), data.begin() + currentSize);
}

template <typename T>
const T* ManagedBuffer<T>::begin() const {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " begin() called without ensureHostBufferPopulated()");
#endif
  return data.data();
}

template <typename T>
const T* ManagedBuffer<T>::end() const {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " end() called without ensureHostBufferPopulated()");
#endif
  return data.data() + currentSize;
}

template <typename T>
T ManagedBuffer<T>::getHostValue(size_t ind) const {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " getHostValue() called without ensureHostBufferPopulated()");
#endif
  return data[ind];
}

template <typename T>
T ManagedBuffer<T>::getHostValue(size_t indX, size_t indY) const {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " getHostValue() called without ensureHostBufferPopulated()");
  checkDeviceBufferTypeIs(DeviceBufferType::Texture2d);
#endif
  return getHostValue(sizeY * indX + indY);
}

template <typename T>
T ManagedBuffer<T>::getHostValue(size_t indX, size_t indY, size_t indZ) const {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " getHostValue() called without ensureHostBufferPopulated()");
  checkDeviceBufferTypeIs(DeviceBufferType::Texture3d);
#endif
  return getHostValue(sizeZ * sizeY * indX + sizeZ * indY + indZ);
}

template <typename T>
void ManagedBuffer<T>::setHostValue(size_t ind, T val) {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " setHostValue() called without ensureHostBufferPopulated()");
#endif
  data[ind] = val;
  // NOTE: intentionally no side-effects here. Caller must call markHostBufferUpdated() after all writes are done.
}

template <typename T>
void ManagedBuffer<T>::setHostValue(size_t indX, size_t indY, T val) {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " setHostValue() called without ensureHostBufferPopulated()");
  checkDeviceBufferTypeIs(DeviceBufferType::Texture2d);
#endif
  setHostValue(sizeY * indX + indY, val);
}

template <typename T>
void ManagedBuffer<T>::setHostValue(size_t indX, size_t indY, size_t indZ, T val) {
#ifndef NDEBUG
  if (!hostBufferValid) exception("ManagedBuffer " + name + " setHostValue() called without ensureHostBufferPopulated()");
  checkDeviceBufferTypeIs(DeviceBufferType::Texture3d);
#endif
  setHostValue(sizeZ * sizeY * indX + sizeZ * indY + indZ, val);
}

template <typename T>
void ManagedBuffer<T>::markHostBufferUpdated() {
  hostBufferValid = true;
  deviceBufferValid = false;
  invalidateIndexedViews();
  requestRedraw();
}

template <typename T>
void ManagedBuffer<T>::syncToDeviceIfNeeded() {
  // Quick exit: device data is current and all indexed views are up to date.
  if (deviceBufferValid && indexedViewsValid) return;

  if (!deviceBufferValid && !hostBufferValid) {
    // Neither host nor device has valid data at draw time. This is a bug: either the buffer was
    // never populated, or getRenderAttributeBuffer() was not called before draw (which would have
    // triggered the compute function and set deviceBufferValid).
    exception("ManagedBuffer " + name + " has no valid data on host or device at draw time");
  }

  // Host data is valid. Upload to device if not already current.
  if (!deviceBufferValid) {
    if (renderAttributeBuffer) {
      renderAttributeBuffer->setData(data);
    }
    if (renderTextureBuffer) {
      renderTextureBuffer->setData(data);
    }
    deviceBufferValid = true;
  }
  
  
  // Update indexed views 
  if (!indexedViewsValid) {

    if(deviceBufferValid) { // always true now at this point

      // TODO do an on-device update of indexed views

      // On-device update not implemented yet, copy it to host and do the update from the host
      ensureHostBufferPopulated();
      updateIndexedViews(); // sets indexedViewsValid = true
    }
  }
}

template <typename T>
T ManagedBuffer<T>::getValue(size_t ind) {

  // For the texture case, always copy to the host and pull from there
  if (deviceBufferTypeIsTexture()) {
    ensureHostBufferPopulated();
  }

  if (hostBufferValid) {
    if (ind >= currentSize)
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
  }

  if (isInNeedsComputeState()) {
    computeFunc();
    if (ind >= currentSize)
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
  }

  if (!hostBufferValid && deviceBufferValid) {
    // NOTE: right now this case should never happen unless deviceBufferType == DeviceBufferType::Attribute.
    // In the texture case, we cannot get a single pixel from the backend anyway, so we always
    // call ensureHostBufferPopulated() above and do the host access.

    if (static_cast<int64_t>(ind) >= renderAttributeBuffer->getDataSize())
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");

    return getAttributeBufferData<T>(*renderAttributeBuffer, ind);
  }

  return T(); // dummy return
}

template <typename T>
T ManagedBuffer<T>::getValue(size_t indX, size_t indY) {
  checkDeviceBufferTypeIs(DeviceBufferType::Texture2d);
  // always call the single-indexed version, which will default to a host copy and host lookup
  // we can't grab a single texture pixel from the openGL backend anyway
  return getValue(sizeY * indX + indY);
}

template <typename T>
T ManagedBuffer<T>::getValue(size_t indX, size_t indY, size_t indZ) {
  checkDeviceBufferTypeIs(DeviceBufferType::Texture3d);
  // always call the single-indexed version, which will default to a host copy and host lookup
  // we can't grab a single texture pixel from the openGL backend anyway
  return getValue(sizeZ * sizeY * indX + sizeZ * indY + indZ);
}

template <typename T>
size_t ManagedBuffer<T>::size() const {
  return currentSize;
}

template <typename T>
DeviceBufferType ManagedBuffer<T>::getDeviceBufferType() const {
  return deviceBufferType;
}

template <typename T>
std::string ManagedBuffer<T>::summaryString() const {

  std::string str = "";

  str += "[" + name + "]";
  str += "  hostValid: " + std::string(hostBufferValid ? "T" : "F");
  str += "  deviceValid: " + std::string(deviceBufferValid ? "T" : "F");
  str += "  indexedViewsValid: " + std::string(indexedViewsValid ? "T" : "F");
  str += "  hasComputeFunc: " + std::string(dataGetsComputed ? "T" : "F");
  str += "  size: " + std::to_string(currentSize) + " / " + std::to_string(managedCapacity);
  str += "  device type: ";
  switch (deviceBufferType) {
  case DeviceBufferType::Attribute:
    str += "Attribute";
    break;
  case DeviceBufferType::Texture1d:
    str += "Texture1d";
    break;
  case DeviceBufferType::Texture2d:
    str += "Texture2d";
    break;
  case DeviceBufferType::Texture3d:
    str += "Texture3d";
    break;
  }

  return str;
}


template <typename T>
void ManagedBuffer<T>::recomputeIfPopulated() {
  if (!dataGetsComputed) { // sanity check
    exception("called recomputeIfPopulated() on buffer which does not get computed");
  }

  // if not populated (NeedsCompute state), quick out
  if (!hostBufferValid && !deviceBufferValid) {
    return;
  }

  invalidateHostBuffer();
  computeFunc();
  markHostBufferUpdated();
}

template <typename T>
std::shared_ptr<render::AttributeBuffer> ManagedBuffer<T>::getRenderAttributeBuffer() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  if (!renderAttributeBuffer) {
    ensureHostBufferPopulated();
    renderAttributeBuffer = generateAttributeBuffer<T>(render::engine);
    renderAttributeBuffer->reserveCapacity(managedCapacity);
    renderAttributeBuffer->setData(data);
    deviceBufferValid = true;
  }
  return renderAttributeBuffer;
}

template <typename T>
std::shared_ptr<render::TextureBuffer> ManagedBuffer<T>::getRenderTextureBuffer() {
  checkDeviceBufferTypeIsTexture();

  if (!renderTextureBuffer) {
    ensureHostBufferPopulated();
    renderTextureBuffer = generateTextureBuffer<T>(deviceBufferType, render::engine);

    // templatize this?
    switch (deviceBufferType) {
    case DeviceBufferType::Attribute:
      exception("bad call");
      break;
    case DeviceBufferType::Texture1d:
      // Allocate GPU texture at the full managed capacity so future within-capacity resizes
      // don't require a new GPU buffer. setData() below uploads only data.size() elements.
      renderTextureBuffer->resize(static_cast<uint32_t>(managedCapacity));
      break;
    case DeviceBufferType::Texture2d:
      renderTextureBuffer->resize(sizeX, sizeY);
      break;
    case DeviceBufferType::Texture3d:
      renderTextureBuffer->resize(sizeX, sizeY, sizeZ);
      break;
    }

    renderTextureBuffer->setData(data);
    deviceBufferValid = true;
  }
  return renderTextureBuffer;
}

template <typename T>
void ManagedBuffer<T>::markRenderAttributeBufferUpdated() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  invalidateHostBuffer(); // also clears data
  deviceBufferValid = true;
  invalidateIndexedViews();
  requestRedraw();
}

template <typename T>
void ManagedBuffer<T>::markRenderTextureBufferUpdated() {
  checkDeviceBufferTypeIsTexture();

  invalidateHostBuffer(); // also clears data
  deviceBufferValid = true;
  invalidateIndexedViews();
  requestRedraw();
}

template <typename T>
std::shared_ptr<render::AttributeBuffer>
ManagedBuffer<T>::getIndexedRenderAttributeBuffer(ManagedBuffer<uint32_t>& indices) {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  removeDeletedIndexedViews(); // periodic filtering

  // Check if we have already created this indexed view, and if so just return it
  for (std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>& existingViewTup :
       existingIndexedViews) {

    // both the cache-key source index ptr and the view buffer ptr must still be alive (and the index must match)
    // note that we can't verify that the index buffer is still alive, you will just get memory errors here if it
    // has been deleted
    std::shared_ptr<render::AttributeBuffer> viewBufferPtr = std::get<1>(existingViewTup).lock();
    if (viewBufferPtr) {
      render::ManagedBuffer<uint32_t>& indexBufferCand = *(std::get<0>(existingViewTup));
      if (indexBufferCand.uniqueID == indices.uniqueID) {
        return viewBufferPtr;
      }
    }
  }

  // We don't have it. Create a new one and return that.
  ensureHostBufferPopulated();
  std::shared_ptr<render::AttributeBuffer> newBuffer = generateAttributeBuffer<T>(render::engine);
  indices.ensureHostBufferPopulated();
  std::vector<T> expandData = gather(data, indices.data);
  newBuffer->setData(expandData); // initially populate
  existingIndexedViews.emplace_back(&indices, newBuffer);

  return newBuffer;
}

template <typename T>
std::vector<T> ManagedBuffer<T>::getIndexedView(ManagedBuffer<uint32_t>& indices) {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);
  ensureHostBufferPopulated();
  indices.ensureHostBufferPopulated();
  return gather(data, indices.data);
}

template <typename T>
void ManagedBuffer<T>::updateIndexedViews() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  removeDeletedIndexedViews(); // periodic filtering

  for (std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>& existingViewTup :
       existingIndexedViews) {

    std::shared_ptr<render::AttributeBuffer> viewBufferPtr = std::get<1>(existingViewTup).lock();
    if (!viewBufferPtr) continue; // skip if it has been deleted (will be removed eventually)

    // note: index buffer must still be alive here. we can't check it, you will just get memory errors
    // if it has been deleted
    render::ManagedBuffer<uint32_t>& indices = *std::get<0>(existingViewTup);
    render::AttributeBuffer& viewBuffer = *viewBufferPtr;

    // apply the indexing and set the data
    indices.ensureHostBufferPopulated();
    std::vector<T> expandData = gather(data, indices.data);
    viewBuffer.setData(expandData);

    // NOTE: this updates ALL registered indexed views of this buffer, even if only one of them is
    // actually used in the current draw call. This is overly eager but correct; a lazier design
    // would require indexed views to be first-class ManagedBuffers with their own NeedsCompute state.

    // TODO: add direct GPU-side indexed copy support (without round-tripping through the host) using a
    // transform-feedback/compute shader program, to avoid the CPU gather cost for GPU-resident buffers.
  }

  indexedViewsValid = true;
  requestRedraw();
}

template <typename T>
void ManagedBuffer<T>::removeDeletedIndexedViews() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  // "erase-remove idiom"
  // (remove list entries for which the view weak_ptr has .expired() == true)
  existingIndexedViews.erase(
      std::remove_if(
          existingIndexedViews.begin(), existingIndexedViews.end(),
          [](const std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>& entry)
              -> bool { return std::get<1>(entry).expired(); }),
      existingIndexedViews.end());
}

template <typename T>
void ManagedBuffer<T>::invalidateHostBuffer() {
  ManagedBufferBase::invalidateHostBuffer();
  data.clear();
}

template <typename T>
void ManagedBuffer<T>::invalidateIndexedViews() {
  if (existingIndexedViews.size() > 0) indexedViewsValid = false;
}

template <typename T>
typename ManagedBuffer<T>::CanonicalDataSource ManagedBuffer<T>::currentCanonicalDataSource() {

  // Always prefer the host data if it is up to date
  if (hostBufferValid) {
    return CanonicalDataSource::HostData;
  }

  // Check if the render buffer contains the canonical data
  if (deviceBufferValid) {
    return CanonicalDataSource::RenderBuffer;
  }

  if (dataGetsComputed) {
    return CanonicalDataSource::NeedsCompute;
  }

  // error! should always be one of the above
  exception("ManagedBuffer " + name +
            " does not have a data in either host or device buffers, nor a compute function.");
  return CanonicalDataSource::HostData; // dummy return
}


// === Interact with the buffer registry

std::tuple<bool, ManagedBufferType> ManagedBufferRegistry::hasManagedBufferType(std::string name) {

  // clang-format off

  if (hasManagedBuffer<float>(name))  return std::make_tuple(true, ManagedBufferType::Float);
  if (hasManagedBuffer<double>(name)) return std::make_tuple(true, ManagedBufferType::Double);

  if (hasManagedBuffer<glm::vec2>(name)) return std::make_tuple(true, ManagedBufferType::Vec2);
  if (hasManagedBuffer<glm::vec3>(name)) return std::make_tuple(true, ManagedBufferType::Vec3);
  if (hasManagedBuffer<glm::vec4>(name)) return std::make_tuple(true, ManagedBufferType::Vec4);
  
  if (hasManagedBuffer<std::array<glm::vec3,2>>(name)) return std::make_tuple(true, ManagedBufferType::Arr2Vec3);
  if (hasManagedBuffer<std::array<glm::vec3,3>>(name)) return std::make_tuple(true, ManagedBufferType::Arr3Vec3);
  if (hasManagedBuffer<std::array<glm::vec3,4>>(name)) return std::make_tuple(true, ManagedBufferType::Arr4Vec3);
  
  if (hasManagedBuffer<int32_t>(name))  return std::make_tuple(true, ManagedBufferType::Int32);
  if (hasManagedBuffer<glm::ivec2>(name)) return std::make_tuple(true, ManagedBufferType::IVec2);
  if (hasManagedBuffer<glm::ivec3>(name)) return std::make_tuple(true, ManagedBufferType::IVec3);
  if (hasManagedBuffer<glm::ivec4>(name)) return std::make_tuple(true, ManagedBufferType::IVec4);

  if (hasManagedBuffer<uint32_t>(name)) return std::make_tuple(true, ManagedBufferType::UInt32);
  if (hasManagedBuffer<glm::uvec2>(name)) return std::make_tuple(true, ManagedBufferType::UVec2);
  if (hasManagedBuffer<glm::uvec3>(name)) return std::make_tuple(true, ManagedBufferType::UVec3);
  if (hasManagedBuffer<glm::uvec4>(name)) return std::make_tuple(true, ManagedBufferType::UVec4);

  // clang-format on

  return std::make_tuple(false, ManagedBufferType::Float);
}

// === Explicit template instantiation for the supported types

// Attribute versions

template class ManagedBuffer<float>;
template class ManagedBuffer<double>;

template class ManagedBuffer<glm::vec2>;
template class ManagedBuffer<glm::vec3>;
template class ManagedBuffer<glm::vec4>;

template class ManagedBuffer<std::array<glm::vec3, 2>>;
template class ManagedBuffer<std::array<glm::vec3, 3>>;
template class ManagedBuffer<std::array<glm::vec3, 4>>;

template class ManagedBuffer<int32_t>;
template class ManagedBuffer<glm::ivec2>;
template class ManagedBuffer<glm::ivec3>;
template class ManagedBuffer<glm::ivec4>;

template class ManagedBuffer<uint32_t>;
template class ManagedBuffer<glm::uvec2>;
template class ManagedBuffer<glm::uvec3>;
template class ManagedBuffer<glm::uvec4>;

// Buffer maps

template struct ManagedBufferMap<float>;
template struct ManagedBufferMap<double>;

template struct ManagedBufferMap<glm::vec2>;
template struct ManagedBufferMap<glm::vec3>;
template struct ManagedBufferMap<glm::vec4>;

template struct ManagedBufferMap<std::array<glm::vec3, 2>>;
template struct ManagedBufferMap<std::array<glm::vec3, 3>>;
template struct ManagedBufferMap<std::array<glm::vec3, 4>>;

template struct ManagedBufferMap<int32_t>;
template struct ManagedBufferMap<glm::ivec2>;
template struct ManagedBufferMap<glm::ivec3>;
template struct ManagedBufferMap<glm::ivec4>;

template struct ManagedBufferMap<uint32_t>;
template struct ManagedBufferMap<glm::uvec2>;
template struct ManagedBufferMap<glm::uvec3>;
template struct ManagedBufferMap<glm::uvec4>;


// clang-format off

template<> ManagedBufferMap<float>&                    ManagedBufferMap<float>::getManagedBufferMapRef                   (ManagedBufferRegistry* r) { return r->managedBufferMap_float; }
template<> ManagedBufferMap<double>&                   ManagedBufferMap<double>::getManagedBufferMapRef                  (ManagedBufferRegistry* r) { return r->managedBufferMap_double; }
template<> ManagedBufferMap<glm::vec2>&                ManagedBufferMap<glm::vec2>::getManagedBufferMapRef               (ManagedBufferRegistry* r) { return r->managedBufferMap_vec2; }
template<> ManagedBufferMap<glm::vec3>&                ManagedBufferMap<glm::vec3>::getManagedBufferMapRef               (ManagedBufferRegistry* r) { return r->managedBufferMap_vec3; }
template<> ManagedBufferMap<glm::vec4>&                ManagedBufferMap<glm::vec4>::getManagedBufferMapRef               (ManagedBufferRegistry* r) { return r->managedBufferMap_vec4; }
template<> ManagedBufferMap<std::array<glm::vec3,2>>&  ManagedBufferMap<std::array<glm::vec3,2>>::getManagedBufferMapRef (ManagedBufferRegistry* r) { return r->managedBufferMap_arr2vec3; }
template<> ManagedBufferMap<std::array<glm::vec3,3>>&  ManagedBufferMap<std::array<glm::vec3,3>>::getManagedBufferMapRef (ManagedBufferRegistry* r) { return r->managedBufferMap_arr3vec3; }
template<> ManagedBufferMap<std::array<glm::vec3,4>>&  ManagedBufferMap<std::array<glm::vec3,4>>::getManagedBufferMapRef (ManagedBufferRegistry* r) { return r->managedBufferMap_arr4vec3; }
template<> ManagedBufferMap<int32_t>&                  ManagedBufferMap<int32_t>::getManagedBufferMapRef                 (ManagedBufferRegistry* r) { return r->managedBufferMap_int32; }
template<> ManagedBufferMap<glm::ivec2>&               ManagedBufferMap<glm::ivec2>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_ivec2; }
template<> ManagedBufferMap<glm::ivec3>&               ManagedBufferMap<glm::ivec3>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_ivec3; }
template<> ManagedBufferMap<glm::ivec4>&               ManagedBufferMap<glm::ivec4>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_ivec4; }
template<> ManagedBufferMap<uint32_t>&                 ManagedBufferMap<uint32_t>::getManagedBufferMapRef                (ManagedBufferRegistry* r) { return r->managedBufferMap_uint32; }
template<> ManagedBufferMap<glm::uvec2>&               ManagedBufferMap<glm::uvec2>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_uvec2; }
template<> ManagedBufferMap<glm::uvec3>&               ManagedBufferMap<glm::uvec3>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_uvec3; }
template<> ManagedBufferMap<glm::uvec4>&               ManagedBufferMap<glm::uvec4>::getManagedBufferMapRef              (ManagedBufferRegistry* r) { return r->managedBufferMap_uvec4; }

// clang-format on

} // namespace render

std::string typeName(ManagedBufferType type) { return enum_to_string(type); };

} // namespace polyscope

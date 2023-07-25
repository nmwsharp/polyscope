// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include <vector>

#include "polyscope/render/managed_buffer.h"

#include "polyscope/internal.h"
#include "polyscope/messages.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/templated_buffers.h"

namespace polyscope {
namespace render {

template <typename T>
ManagedBuffer<T>::ManagedBuffer(const std::string& name_, std::vector<T>& data_)
    : name(name_), uniqueID(internal::getNextUniqueID()), data(data_), dataGetsComputed(false),
      hostBufferIsPopulated(true) {}


template <typename T>
ManagedBuffer<T>::ManagedBuffer(const std::string& name_, std::vector<T>& data_, std::function<void()> computeFunc_)
    : name(name_), uniqueID(internal::getNextUniqueID()), data(data_), dataGetsComputed(true),
      computeFunc(computeFunc_), hostBufferIsPopulated(false) {}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_) {
  if (deviceBufferType != DeviceBufferType::Attribute) exception("managed buffer can only be set as texture once");

  deviceBufferType = DeviceBufferType::Texture1d;
  sizeX = sizeX_;
}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_, uint32_t sizeY_) {
  if (deviceBufferType != DeviceBufferType::Attribute) exception("managed buffer can only be set as texture once");

  deviceBufferType = DeviceBufferType::Texture2d;
  sizeX = sizeX_;
  sizeY = sizeY_;
}

template <typename T>
void ManagedBuffer<T>::setTextureSize(uint32_t sizeX_, uint32_t sizeY_, uint32_t sizeZ_) {
  if (deviceBufferType != DeviceBufferType::Attribute) exception("managed buffer can only be set as texture once");

  deviceBufferType = DeviceBufferType::Texture3d;
  sizeX = sizeX_;
  sizeY = sizeY_;
  sizeZ = sizeZ_;
}

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

    if (deviceBufferTypeIsTexture()) {
      if (!renderTextureBuffer) exception("render buffer should be allocated but isn't");

      // copy the data back from the renderBuffer
      // TODO not implemented yet
      exception("copy-back from texture not implemented yet");
    } else {
      // sanity check
      if (!renderAttributeBuffer) exception("render buffer should be allocated but isn't");

      // copy the data back from the renderBuffer
      data = getAttributeBufferDataRange<T>(*renderAttributeBuffer, 0, renderAttributeBuffer->getDataSize());
    }

    break;
  };
}

template <typename T>
std::vector<T>& ManagedBuffer<T>::getPopulatedHostBufferRef() {
  ensureHostBufferPopulated();
  return data;
}

template <typename T>
void ManagedBuffer<T>::markHostBufferUpdated() {
  hostBufferIsPopulated = true;

  // If the data is stored in the device-side buffers, update it as needed
  if (renderAttributeBuffer) {
    renderAttributeBuffer->setData(data);
    requestRedraw();
  }
}

template <typename T>
T ManagedBuffer<T>::getValue(size_t ind) {

  // For the texture case, always copy to the host and pull from there
  if (deviceBufferTypeIsTexture()) {
    ensureHostBufferPopulated();
  }

  switch (currentCanonicalDataSource()) {
  case CanonicalDataSource::HostData:
    if (ind >= data.size())
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::NeedsCompute:
    computeFunc();
    if (ind >= data.size())
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");
    return data[ind];
    break;

  case CanonicalDataSource::RenderBuffer:

    // NOTE: right now this case should never happen unless deviceBufferType == DeviceBufferType::Attribute.
    // In the texture case, we cannot get a single pixel from the backend anyway, so we always
    // call ensureHostBufferPopulated() above and do the host access.

    if (static_cast<int64_t>(ind) >= renderAttributeBuffer->getDataSize())
      exception("out of bounds access in ManagedBuffer " + name + " getValue(" + std::to_string(ind) + ")");

    T val = getAttributeBufferData<T>(*renderAttributeBuffer, ind);
    return val;
    break;
  };

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
size_t ManagedBuffer<T>::size() {

  switch (currentCanonicalDataSource()) {
  case CanonicalDataSource::HostData:
    return data.size();
    break;

  case CanonicalDataSource::NeedsCompute:
    return 0;
    break;

  case CanonicalDataSource::RenderBuffer:
    if (deviceBufferType == DeviceBufferType::Attribute) {
      return renderAttributeBuffer->getDataSize();
    } else {
      return sizeX * sizeY * sizeZ;
    }
    break;
  };

  return INVALID_IND;
}

template <typename T>
bool ManagedBuffer<T>::hasData() {

  if (hostBufferIsPopulated) return true;
  if (deviceBufferType == DeviceBufferType::Attribute && renderAttributeBuffer) return true;
  if (deviceBufferType == DeviceBufferType::Texture1d && renderTextureBuffer) return true;
  if (deviceBufferType == DeviceBufferType::Texture2d && renderTextureBuffer) return true;
  if (deviceBufferType == DeviceBufferType::Texture3d && renderTextureBuffer) return true;
  return false;
}

template <typename T>
void ManagedBuffer<T>::recomputeIfPopulated() {
  if (!dataGetsComputed) { // sanity check
    exception("called recomputeIfPopulated() on buffer which does not get computed");
  }

  // if not populated, quick out
  if (currentCanonicalDataSource() == CanonicalDataSource::NeedsCompute) {
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
    ensureHostBufferPopulated(); // warning: the order of these matters because of how hostBufferPopulated works
    renderAttributeBuffer = generateAttributeBuffer<T>(render::engine);
    renderAttributeBuffer->setData(data);
  }
  return renderAttributeBuffer;
}

template <typename T>
std::shared_ptr<render::TextureBuffer> ManagedBuffer<T>::getRenderTextureBuffer() {
  checkDeviceBufferTypeIsTexture();

  if (!renderTextureBuffer) {
    ensureHostBufferPopulated(); // warning: the order of these matters because of how hostBufferPopulated works

    renderTextureBuffer = generateTextureBuffer<T>(deviceBufferType, render::engine);
    renderTextureBuffer->setData(data);
  }
  return renderTextureBuffer;
}

template <typename T>
void ManagedBuffer<T>::markRenderAttributeBufferUpdated() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  invalidateHostBuffer();
  updateIndexedViews();
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

    // TODO fornow, only CPU-side updating is supported. Add direct GPU-side support using the bufferIndexCopyProgram
    // below.
  }
}

template <typename T>
void ManagedBuffer<T>::removeDeletedIndexedViews() {
  checkDeviceBufferTypeIs(DeviceBufferType::Attribute);

  // "erase-remove idiom"
  // (remove list entries for which the view weak_ptr has .expired() == true
  existingIndexedViews.erase(
      std::remove_if(
          existingIndexedViews.begin(), existingIndexedViews.end(),
          [&](const std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>& entry)
              -> bool { return std::get<1>(entry).expired(); }),
      existingIndexedViews.end());
}

template <typename T>
void ManagedBuffer<T>::invalidateHostBuffer() {
  hostBufferIsPopulated = false;
  data.clear();
}

template <typename T>
bool ManagedBuffer<T>::deviceBufferTypeIsTexture() {
  return ((deviceBufferType == DeviceBufferType::Texture1d) || (deviceBufferType == DeviceBufferType::Texture2d) ||
          (deviceBufferType == DeviceBufferType::Texture3d));
}

template <typename T>
void ManagedBuffer<T>::checkDeviceBufferTypeIs(DeviceBufferType targetType) {
  if (targetType != deviceBufferType) {
    exception("ManagedBuffer has wrong type for this operation. Expected " + deviceBufferTypeName(targetType) +
              " but is " + deviceBufferTypeName(deviceBufferType));
  }
}

template <typename T>
void ManagedBuffer<T>::checkDeviceBufferTypeIsTexture() {
  if (!deviceBufferTypeIsTexture()) {
    exception("ManagedBuffer has wrong type for this operation. Expected a Texture1d/2d/3d but is " +
              deviceBufferTypeName(deviceBufferType));
  }
}

template <typename T>
typename ManagedBuffer<T>::CanonicalDataSource ManagedBuffer<T>::currentCanonicalDataSource() {

  // Always prefer the host data if it is up to date
  if (hostBufferIsPopulated) {
    return CanonicalDataSource::HostData;
  }

  // Check if the render buffer contains the canonical data
  if (renderAttributeBuffer) {
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


template <typename T>
void ManagedBuffer<T>::ensureHaveBufferIndexCopyProgram() {
  if (bufferIndexCopyProgram) return;

  // sanity check
  if (!renderAttributeBuffer) exception("ManagedBuffer " + name + " asked to copy indices, but has no buffers");

  // TODO allocate the transform feedback program
}

template <typename T>
void ManagedBuffer<T>::invokeBufferIndexCopyProgram() {
  ensureHaveBufferIndexCopyProgram();
  bufferIndexCopyProgram->draw();
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

template class ManagedBuffer<uint32_t>;
template class ManagedBuffer<int32_t>;

template class ManagedBuffer<glm::uvec2>;
template class ManagedBuffer<glm::uvec3>;
template class ManagedBuffer<glm::uvec4>;


} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"
#include "polyscope/weak_handle.h"

namespace polyscope {
namespace render {

// forward declaration
class ManagedBufferRegistry;

/*
 * Non-templated base class for ManagedBuffer<T>.
 * Contains all type-independent members, in particular the syncToDeviceIfNeeded() function which needs
 * to be called without the type known.
 */
class ManagedBufferBase {
public:
  virtual ~ManagedBufferBase() = default;

  // Called by ShaderProgram::draw() before issuing the GPU draw call.
  // Pushes host data to device if deviceBufferValid is false and hostBufferValid is true.
  virtual void syncToDeviceIfNeeded() = 0;

  const std::string name;
  const uint64_t uniqueID;

protected:
  // protected constructors — only ManagedBuffer<T> should construct this
  ManagedBufferBase(ManagedBufferRegistry* registry_, const std::string& name_, bool dataGetsComputed_,
                    bool hostBufferValid_);

  // Internal helpers that don't depend on T
  void invalidateHostBuffer();
  bool deviceBufferTypeIsTexture() const;
  void checkDeviceBufferTypeIs(DeviceBufferType targetType) const;
  void checkDeviceBufferTypeIsTexture() const;

  // True when the buffer has no valid data on host or device and is waiting to be computed.
  bool isInNeedsComputeState() const { return !hostBufferValid && !deviceBufferValid && dataGetsComputed; }

  ManagedBufferRegistry* registry;

  bool dataGetsComputed;             // if true, the value gets computed on-demand by calling computeFunc()
  std::function<void()> computeFunc; // (optional) callback which populates the buffer

  bool hostBufferValid;   // true if host-side data is current
  bool deviceBufferValid; // true if device-side buffer matches current host data

  // True if all registered indexed views are consistent with the current source data.
  // Gets marked as false whenever the source data changes on the host or device, until the indexed views are updated
  bool indexedViewsValid = true;

  // Explicit size and capacity.
  // Invariant: when hostBufferValid, data.size() == managedCapacity (the vector is always kept at full
  // capacity — data.capacity() (std::vector internal) is never relied on for anything).
  // currentSize <= managedCapacity is the number of logically valid elements.
  size_t managedCapacity = 0;
  size_t currentSize = 0;

  std::shared_ptr<render::AttributeBuffer> renderAttributeBuffer;
  std::shared_ptr<render::TextureBuffer> renderTextureBuffer;

  DeviceBufferType deviceBufferType = DeviceBufferType::Attribute;

  uint32_t sizeX = 0; // only populated if texture
  uint32_t sizeY = 0; // only populated if texture
  uint32_t sizeZ = 0; // only populated if texture
};


/*
 * This class owns and manages a typed data buffer in Polyscope, handling common data-management concerns of:
 *
 *      (a) mirroring the buffer to the GPU/rendering framework
 *      (b) allowing external users to update data either on the CPU- or GPU-side, and updating mirrored copies
 * appropriately
 *      (c) managing _indexed_ data, which gets expanded according to some index set for access at rendering time.
 *
 * Most often this class is used to hold structure/quantity data passed in by the user, such as a scalar quantity, but
 * it also sometimes holds automatically-computed values within Polyscope, such as a vertex normal buffer for
 * rendering.
 *
 * Use the public accessor API (getHostValue, setHostValue, setDataHost, begin/end, etc.) to interact with the buffer.
 */
template <typename T>
class ManagedBuffer : public ManagedBufferBase, public virtual WeakReferrable {
public:
  // ========================================================================
  // == Constructors
  // ========================================================================

  // Create an empty buffer with no data. Use resize()+setHostValue() or setDataHost() to populate.
  ManagedBuffer(ManagedBufferRegistry* registry, const std::string& name);

  // Manage a buffer of data which is explicitly set externally.
  ManagedBuffer(ManagedBufferRegistry* registry, const std::string& name, std::vector<T> data);

  // Manage a buffer of data which gets computed lazily
  ManagedBuffer(ManagedBufferRegistry* registry, const std::string& name, std::function<void()> computeFunc);

  ~ManagedBuffer();


  // ========================================================================
  // == Size, capacity, and type management
  // ========================================================================

  // The .size() of the buffer is the number of data elements it holds.
  //
  // The .capacity() of the buffer is the number of data elements it has capacity for without needing to reallocate
  // (similar to std::vector).
  //
  // In a simple flow where we put data in a buffer once, the capacity is the same as the size and neither ever changes.
  // But in settings where we e.g. incrementally add elements or change the number of data elements on each frame, we
  // may want to allocate a larger capacity to avoid expensive re-allocation each time.

  // Get the size of the data in the buffer. Always <= capacity().
  size_t size() const;

  // Get the capacity of the buffer. The maximum size the buffer can be resized to without triggering a reallocation.
  // Always >= size().
  size_t capacity() const;

  // Set the managed capacity to newCapacity. Unlike resize(), which grows capacity via amortized
  // doubling, this sets the logical capacity to a precise value. Error if newCapacity < size().
  // Reallocates the GPU buffer in-place (same buffer object, new backing memory) if one exists.
  //
  // Valid for attributes and 1D textures only; multidimensional textures always have capacity
  // equal to their size, so use setTextureSize() instead.
  void setCapacity(size_t newCapacity);

  // Set the device buffer type. Call once at construction time before setTextureSize().
  // All buffers are Attribute by default, and can be switch to texture exactly once.
  void setAsType(DeviceBufferType type);

  // Is it an attribute, texture1d, texture2d, etc?
  DeviceBufferType getDeviceBufferType() const;

  // Set or update the texture dimensions. On first call (managedCapacity == 0): just stores the
  // dimensions. On subsequent calls: no-op if unchanged; otherwise copies any device-side data
  // back to host and reallocates the GPU buffer in-place.
  // Requires setAsType() to have been called first with the matching texture type.
  // For 1D textures, resize() may be used instead.
  void setTextureSize(uint32_t sizeX);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ);
  std::array<uint32_t, 3> getTextureSize() const;


  // Resize the buffer to newSize elements. Sets hostBufferValid = true.
  //
  // If newSize <= capacity(), this is a cheap constant-time operation which just updates metadata.
  //
  // If newSize > capacity(), this triggers a full reallocation. The new capacity is set to at least
  // 2*oldCapacity (amortized doubling). Any data that was GPU-resident is first copied back to the
  // host, then the GPU buffer is reallocated in-place (same buffer object, new backing memory).
  // ShaderPrograms holding a reference to the GPU buffer remain valid.
  //
  // Returns true if a reallocation occurred, false if the resize stayed within capacity.
  //
  // Valid for attributes and 1D textures only; call setTextureSize() for multidimensional textures.
  bool resize(size_t newSize);


  // ========================================================================
  // == Basic data access
  // ========================================================================

  // The functions are always valid to call, but might be expensive to call in a type loop or if the data resides
  // only on the device.

  // Get the value at index `i`. It may be dynamically fetched from either the cpu-side `data` member or the render
  // buffer, depending on where the data currently lives.
  // If the data lives only on the device-side render buffer, this function is expensive, so don't call it in a
  // loop.
  T getValue(size_t ind);
  T getValue(size_t indX, size_t indY);              // only valid for 2d texture data
  T getValue(size_t indX, size_t indY, size_t indZ); // only valid for 3d texture data


  // Copy newData into the host buffer. Errors if newData.size() > capacity() — call resize() or
  // setCapacity() first if needed. Does not change capacity. Automatically syncs to any allocated
  // device buffers and triggers a redraw. Prefer this over setHostValue() for bulk writes.
  void setDataHost(const std::vector<T>& newData);

  // Returns a full copy of the host data, populating it from device if needed.
  std::vector<T> getDataCopy();


  // ========================================================================
  // == Low-level data access
  // ========================================================================

  // These functions provide direct access to the underlying host side buffer.
  // Callers are generally _required_ to call ensureHostBufferPopulated() before using these, and
  // markHostBufferUpdated() after making any changes, to maintain the validity of the host/device mirroring.

  // Ensure that the host buffer is populated with the current values. In the common case where the user sets data
  // and it never changes, this does nothing. However, if the value is being updated directly from GPU memory, this
  // mirrors the updates to the host-side buffer. Also, if the value is lazily computed by computeFunc(), it ensures
  // that function has been called.
  void ensureHostBufferPopulated();

  // If the contents of the host buffer have been updated via the functions below, this must be called once updates are
  // finished. It internally handles concerns like reflecting updates to the render buffer.
  void markHostBufferUpdated();

  // Single-element read. Caller MUST call ensureHostBufferPopulated() first.
  T getHostValue(size_t ind) const;
  T getHostValue(size_t indX, size_t indY) const;              // only valid for 2d texture data
  T getHostValue(size_t indX, size_t indY, size_t indZ) const; // only valid for 3d texture data

  // Single-element write. Caller MUST call ensureHostBufferPopulated() first, and MUST call markHostBufferUpdated()
  // after all writes are complete.
  void setHostValue(size_t ind, T val);
  void setHostValue(size_t indX, size_t indY, T val);              // only valid for 2d texture data
  void setHostValue(size_t indX, size_t indY, size_t indZ, T val); // only valid for 3d texture data

  // Raw pointer iteration support. Caller MUST call ensureHostBufferPopulated() before using these.
  const T* begin() const;
  const T* end() const;

  // ========================================================================
  // == Misc meta functions and data management
  // ========================================================================

  // If computeFunc() has already been called to populate the stored data, call it again to recompute the data, and
  // re-fill the buffer if necessary. This function is only meaningful in the case where `dataGetsComputed = true`.
  void recomputeIfPopulated();

  // Sync host data to the device buffer if not up to date
  void syncToDeviceIfNeeded() override;

  // Get an info string for debugging
  std::string summaryString() const;

  // Throw exception if the buffer contains any values which are NaN or Inf as in polyscope::isInvalidValue().
  void checkInvalidValues();

  // ========================================================================
  // == Indexed views
  // ========================================================================

  // For some data (e.g. values a vertices of a mesh), we store the data in a canonical ordering (one value per
  // vertex), but need to index it at render time according to some specified list of indices (one value per
  // triangle corner). Rendering frameworks _do_ have explicit support for this case (e.g. IndexedArray DrawMode in
  // openGL), but for various reasons it does not make sense to use those features. Instead, we explicitly expand
  // out the data into a render buffer for drawing as view[i] = data[indices[i]].
  //
  // If such an index is used, it should be set to the `indices` argument below, and this class will automatically
  // handle expanding out the indexed data to populate the returned buffer. External callers can still update the
  // data directly on the host, and this class will handle updating an indexed version of the data for drawing.
  //
  // Internally, these indexed views are cached. It is safe to call this function many times, after the first the
  // same view will be returned repeatedly at no additional cost.
  std::shared_ptr<render::AttributeBuffer> getIndexedRenderAttributeBuffer(ManagedBuffer<uint32_t>& indices);

  // Get a copy of the data viewed through an index, such that view[i] = data[indices[i]].
  //
  // This follows the same logic as above, but rather than returning a render buffer it simply returns a host-side
  // copy (which is not cached).
  std::vector<T> getIndexedView(ManagedBuffer<uint32_t>& indices);


  // ========================================================================
  // == Direct access to the GPU (device-side) render buffers
  // ========================================================================

  // NOTE: this is only for attribute-accessed buffers (DeviceBufferType::Attribute). See the variants below for
  // textures.

  // NOTE: This class follows a lazy-sync policy: once the render buffer is allocated, it is kept up to date
  // with host-side changes lazily — the actual GPU upload happens in syncToDeviceIfNeeded(), which is called
  // by ShaderProgram::draw() just before the draw call. External writes to the GPU buffer (via
  // markRenderAttributeBufferUpdated()) invalidate the host copy until ensureHostBufferPopulated() is called.

  // Get a reference to the underlying GPU-side attribute buffer
  // Once this reference is created, it will always be immediately updated to reflect any external changes to the
  // data. (note that if you write to this buffer externally, you MUST call markRenderAttributeBufferUpdated()
  // below)
  std::shared_ptr<render::AttributeBuffer> getRenderAttributeBuffer();

  // Tell Polyscope that you wrote updated data into the render buffer. This MUST be called after externally writing
  // to the buffer from getRenderBuffer() above.
  void markRenderAttributeBufferUpdated();

  // NOTE: these follow the same semantics as the attribute version above, but these apply when the buffer is a texture
  // (DeviceBufferType::Texture1d, etc).
  std::shared_ptr<render::TextureBuffer> getRenderTextureBuffer();
  void markRenderTextureBufferUpdated();


protected:

  // Override invalidateHostBuffer to also clear the typed data vector.
  void invalidateHostBuffer();

  // == Internal representation of indexed views
  // NOTE: this seems like a problem, we are storing pointers as keys in a cache. Here, it works out because if the
  // key ptr becomes invalid, the value weak_ptr must also be invalid, and we check that before dereferencing the
  // key.
  std::vector<std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>>
      existingIndexedViews;
  void updateIndexedViews();
  void removeDeletedIndexedViews();
  void invalidateIndexedViews();

  enum class CanonicalDataSource { HostData = 0, NeedsCompute, RenderBuffer };
  CanonicalDataSource currentCanonicalDataSource();

  // TODO: add direct GPU-side indexed copy support using a transform-feedback/compute shader program.
  std::shared_ptr<render::ShaderProgram> bufferIndexCopyProgram;

private:
  template <typename U>
  friend class ManagedBuffer;

  // The raw underlying buffer which this class owns and holds the data.
  //
  // It is possible that data.size() == 0 if the data is lazily computed and has not been computed yet, or if this
  // host-side buffer is invalidated because it is being updated externally directly on the render device.
  //
  // Use the accessor API (getHostValue, setHostValue, setDataHost, begin/end) to interact
  // with this buffer rather than accessing it directly.
  std::vector<T> data;
};


// == Manage a store of all registered managed buffers

// These registries are set up to be static: once a buffer is added it is never removed.

// Helper class representing a map [name] --> [buffer]
template <typename T>
struct ManagedBufferMap {
public:
  void addManagedBuffer(ManagedBuffer<T>* buffer);
  void removeManagedBuffer(ManagedBuffer<T>* buffer);

  ManagedBuffer<T>& getManagedBuffer(std::string name);
  bool hasManagedBuffer(std::string name);

  // internal helper for template things
  static ManagedBufferMap<T>& getManagedBufferMapRef(ManagedBufferRegistry* r);

private:
  // the actual store
  // NOTE: we do NOT support removal
  std::vector<ManagedBuffer<T>*> allBuffers;
};


// A registry of buffer of various types
// Classes (structures, quantities) can extend this to track their own buffers
class ManagedBufferRegistry {
public:
  // get a reference to any buffer that currently exists in Polyscope, by name
  template <typename T>
  ManagedBuffer<T>& getManagedBuffer(std::string name);

  // check for the existence of a managed buffer with a given name
  template <typename T>
  bool hasManagedBuffer(std::string name);

  // checks for a managed buffer with the given name of any type
  // return value is (bool, type), the bool indicates whether the buffer was found, and if so the type indicates what
  // type it was
  std::tuple<bool, ManagedBufferType> hasManagedBufferType(std::string name);

  template <typename T>
  void addManagedBuffer(ManagedBuffer<T>* buffer);

  template <typename T>
  void removeManagedBuffer(ManagedBuffer<T>* buffer);

  // clang-format off
  ManagedBufferMap<float>        managedBufferMap_float;
  ManagedBufferMap<double>       managedBufferMap_double;
  ManagedBufferMap<glm::vec2>    managedBufferMap_vec2;
  ManagedBufferMap<glm::vec3>    managedBufferMap_vec3;
  ManagedBufferMap<glm::vec4>    managedBufferMap_vec4;
  ManagedBufferMap<std::array<glm::vec3,2>> managedBufferMap_arr2vec3;
  ManagedBufferMap<std::array<glm::vec3,3>> managedBufferMap_arr3vec3;
  ManagedBufferMap<std::array<glm::vec3,4>> managedBufferMap_arr4vec3;
  ManagedBufferMap<int32_t>      managedBufferMap_int32;
  ManagedBufferMap<glm::ivec2>   managedBufferMap_ivec2;
  ManagedBufferMap<glm::ivec3>   managedBufferMap_ivec3;
  ManagedBufferMap<glm::ivec4>   managedBufferMap_ivec4;
  ManagedBufferMap<uint32_t>     managedBufferMap_uint32;
  ManagedBufferMap<glm::uvec2>   managedBufferMap_uvec2;
  ManagedBufferMap<glm::uvec3>   managedBufferMap_uvec3;
  ManagedBufferMap<glm::uvec4>   managedBufferMap_uvec4;
  // clang-format on
};

} // namespace render

std::string typeName(ManagedBufferType type);

} // namespace polyscope

#include "polyscope/render/managed_buffer.ipp"

// Implementations of ShaderProgram's ManagedBuffer convenience overloads
// (defined here because they need the full ManagedBuffer<T> definition)
namespace polyscope {
namespace render {

template <typename T>
void ShaderProgram::setAttribute(std::string name, ManagedBuffer<T>& buf) {
  setAttribute(name, buf.getRenderAttributeBuffer(), &buf);
}

template <typename T>
void ShaderProgram::setTextureFromBuffer(std::string name, ManagedBuffer<T>& buf) {
  setTextureFromBuffer(name, buf.getRenderTextureBuffer().get(), &buf);
}

} // namespace render
} // namespace polyscope

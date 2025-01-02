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
 * This class is a wrapper which sits on top of data buffers in Polyscope, and handles common data-management concerns
 * of:
 *
 *      (a) mirroring the buffer to the GPU/rendering framework
 *      (b) allowing external users to update data either on the CPU- or GPU-side, and updating mirrored copies
 * appropriately
 *      (c) managed _indexed_ data, which gts expanded according to some index set for access at rendering time.
 *
 * Most often this class is used to wrap structure/quantity data passed in by the user, such as a scalar quantity, but
 * it is also sometimes wraps automatically-computed values within Polyscope, such as a vertex normal buffer for
 * rendering.
 *
 * This class offers functions and accessors which can (and generally, MUST) be used to interact with the underlying
 * data buffer.
 */
template <typename T>
class ManagedBuffer : public virtual WeakReferrable {
public:
  // === Constructors
  // (second variants are advanced versions which allow creation of multi-dimensional texture values)

  // Manage a buffer of data which is explicitly set externally.
  ManagedBuffer(ManagedBufferRegistry* registry, const std::string& name, std::vector<T>& data);


  // Manage a buffer of data which gets computed lazily
  ManagedBuffer(ManagedBufferRegistry* registry, const std::string& name, std::vector<T>& data,
                std::function<void()> computeFunc);


  ~ManagedBuffer();


  // === Core members

  // A meaningful name for the buffer
  std::string name;

  // A globally-unique ID
  const uint64_t uniqueID;

  // The registry in which it is tracked (can be null)
  ManagedBufferRegistry* registry;


  // The raw underlying buffer which this class wraps that holds the data.
  // It is assumed that it never changes length (although this class may clear it to empty).
  //
  // It is possible that data.size() == 0 if the data is lazily computed and has not been computed yet, or if this
  // host-side buffer is invalidated because it is being updated externally directly on the render device.
  //
  // External users can write directly for this buffer. The required order of operations for writing to this buffer is:
  //    buff.ensureHostBufferAllocated();
  //    buff.data = // fill .data with your values
  //    buff.markHostBufferUpdated();
  //
  //
  std::vector<T>& data;

  // == Members for computed data

  // This class somewhat weirdly tracks data from two separate sources:
  //    A) Values directly stored in `data` by an external source
  //    B) Values that get computed lazily by some function when needed
  //
  // When `dataGetsComputed = true`, we are in Case B, and computeFunc() must be set to a callback that does the
  // lazy computing.

  bool dataGetsComputed;             // if true, the value gets computed on-demand by calling computeFunc()
  std::function<void()> computeFunc; // (optional) callback which populates the `data` buffer

  // sanity check helper
  void checkInvalidValues();

  // mark as texture, set size
  void setTextureSize(uint32_t sizeX);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ);
  std::array<uint32_t, 3> getTextureSize() const;


  // == Members for indexed data

  // == Basic interactions

  // Ensure that the `data` member vector reference is populated with the current values. In the common-case where
  // the user sets data and it never changes, then this function will do nothing. However, if e.g. the value is
  // being updated directly from GPU memory, this will mirror the updates to the cpu-side vector. Also, if the value
  // is lazily computed by computeFunc(), it ensures that that function has been called.
  void ensureHostBufferPopulated();

  // Ensure that the `data` member has the proper size. This does _not_ populate the buffer with any particular data,
  // just ensures it is allocated. It is useful for when an external wants to fill the buffer with data.
  void ensureHostBufferAllocated();

  // Combines calling ensureHostBufferPopulated() and returning a reference to the `data` member
  std::vector<T>& getPopulatedHostBufferRef();

  // If the contents of `data` are updated, this function MUST be called. It internally handles concerns like
  // reflecting updates to the render buffer.
  void markHostBufferUpdated();

  // Get the value at index `i`. It may be dynamically fetched from either the cpu-side `data` member or the render
  // buffer, depending on where the data currently lives.
  // If the data lives only on the device-side render buffer, this function is expensive, so don't call it in a
  // loop.
  T getValue(size_t ind);
  T getValue(size_t indX, size_t indY);              // only valid for 2d texture data
  T getValue(size_t indX, size_t indY, size_t indZ); // only valid for 3d texture data

  // If computeFunc() has already been called to populate the stored data, call it again to recompute the data, and
  // re-fill the buffer if necessary. This function is only meaningful in the case where `dataGetsComputed = true`.
  void recomputeIfPopulated();

  bool hasData(); // true if there is valid data on either the host or device
  size_t size();  // size of the data (number of entries)

  // Is it an attribute, texture1d, texture2d, etc?
  DeviceBufferType getDeviceBufferType();

  std::string summaryString(); // for debugging

  // ========================================================================
  // == Direct access to the GPU (device-side) render attribute buffer
  // ========================================================================

  // NOTE: this is only for attribute-accessed buffers (DeviceBufferType::Attribute). See the variants below for
  // textures.

  // NOTE: This class follows the policy that once the render buffer is allocated, it is always immediately kept
  // updated to reflect any external changes.

  // Get a reference to the underlying GPU-side attribute buffer
  // Once this reference is created, it will always be immediately updated to reflect any external changes to the
  // data. (note that if you write to this buffer externally, you MUST call markRenderAttributeBufferUpdated()
  // below)
  std::shared_ptr<render::AttributeBuffer> getRenderAttributeBuffer();

  // Tell Polyscope that you wrote updated data into the render buffer. This MUST be called after externally writing
  // to the buffer from getRenderBuffer() above.
  void markRenderAttributeBufferUpdated();

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

  // ========================================================================
  // == Direct access to the GPU (device-side) render texture buffer
  // ========================================================================
  //
  // NOTE: these follow the same semantics as the attribute version above, but these apply when the buffer is a texture
  // (DeviceBufferType::Texture1d, etc).

  std::shared_ptr<render::TextureBuffer> getRenderTextureBuffer();
  void markRenderTextureBufferUpdated();


protected:
  // == Internal members

  bool hostBufferIsPopulated; // true if the host buffer contains currently-valid data

  std::shared_ptr<render::AttributeBuffer> renderAttributeBuffer;
  std::shared_ptr<render::TextureBuffer> renderTextureBuffer;

  // For storing as textures

  // For data that can be interpreted as a 1/2/3 dimensional texture
  DeviceBufferType deviceBufferType = DeviceBufferType::Attribute; // this gets set when you call setTextureSize
  uint32_t sizeX = 0;
  uint32_t sizeY = 0; // holds 0 if texture dim < 2
  uint32_t sizeZ = 0; // holds 0 if texture dim < 3


  // == Internal representation of indexed views
  // NOTE: this seems like a problem, we are storing pointers as keys in a cache. Here, it works out because if the
  // key ptr becomes invalid, the value weak_ptr must also be invalid, and we check that before dereferencing the
  // key.
  std::vector<std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>>
      existingIndexedViews;
  void updateIndexedViews();
  void removeDeletedIndexedViews();

  // == Internal helper functions

  void invalidateHostBuffer();
  bool deviceBufferTypeIsTexture();
  void checkDeviceBufferTypeIs(DeviceBufferType targetType);
  void checkDeviceBufferTypeIsTexture();

  enum class CanonicalDataSource { HostData = 0, NeedsCompute, RenderBuffer };
  CanonicalDataSource currentCanonicalDataSource();

  // Manage the program which copies indexed data from the renderBuffer to the indexed views
  void ensureHaveBufferIndexCopyProgram();
  void invokeBufferIndexCopyProgram();
  std::shared_ptr<render::ShaderProgram> bufferIndexCopyProgram;
};


// == Manage a store of all registered managed buffers

// These registries are set up to be static: once a buffer is added it is never removed.

// Helper class representing a map [name] --> [buffer]
template <typename T>
struct ManagedBufferMap {
public:
  void addManagedBuffer(ManagedBuffer<T>* buffer);

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

  // clang-format off
  ManagedBufferMap<float>        managedBufferMap_float;
  ManagedBufferMap<double>       managedBufferMap_double;
  ManagedBufferMap<glm::vec2>    managedBufferMap_vec2;
  ManagedBufferMap<glm::vec3>    managedBufferMap_vec3;
  ManagedBufferMap<glm::vec4>    managedBufferMap_vec4;
  ManagedBufferMap<std::array<glm::vec3,2>> managedBufferMap_arr2vec3;
  ManagedBufferMap<std::array<glm::vec3,3>> managedBufferMap_arr3vec3;
  ManagedBufferMap<std::array<glm::vec3,4>> managedBufferMap_arr4vec3;
  ManagedBufferMap<uint32_t>     managedBufferMap_uint32;
  ManagedBufferMap<int32_t>      managedBufferMap_int32;
  ManagedBufferMap<glm::uvec2>   managedBufferMap_uvec2;
  ManagedBufferMap<glm::uvec3>   managedBufferMap_uvec3;
  ManagedBufferMap<glm::uvec4>   managedBufferMap_uvec4;
  // clang-format on
};

} // namespace render

std::string typeName(ManagedBufferType type);

} // namespace polyscope

#include "polyscope/render/managed_buffer.ipp"

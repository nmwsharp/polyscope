// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"

namespace polyscope {
namespace render {

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
 * it is also somtimes automatically-computed values within Polyscope, suchas a vertex normal buffer for rendering.
 *
 * This class offers functions and accessors which can (and generally, MUST) be used to interact with the underlying
 * data buffer.
 */
template <typename T>
class ManagedBuffer {
public:
  // === Constructors
  // (second variants are advanced versions which allow creation of multi-dimensional texture values)

  // Manage a buffer of data which is explicitly set externally.
  ManagedBuffer(const std::string& name, std::vector<T>& data);


  // Manage a buffer of data which gets computed lazily
  ManagedBuffer(const std::string& name, std::vector<T>& data, std::function<void()> computeFunc);


  ~ManagedBuffer();


  // === Core members

  // A meaningful name for the buffer
  std::string name;
  const uint64_t uniqueID;


  // The raw underlying buffer which this class wraps that holds the data.
  // It is assumed that it never changes length.
  // External users can write directly for this buffer, but must call markHostBufferUpdated() afterward.
  // data.size() == 0 if the data is lazily computed and has not been computed yet, or if this host-side buffer is
  // invalidated because it is being updated externally directly on the render device.
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


  // mark as texture, set size
  void setTextureSize(uint32_t sizeX);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY);
  void setTextureSize(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ);


  // == Members for indexed data

  // == Basic interactions

  // Ensure that the `data` member vector reference is populated with the current values. In the common-case where
  // the user sets data and it never changes, then this function will do nothing. However, if e.g. the value is
  // being updated directly from GPU memory, this will mirror the updates to the cpu-side vector. Also, if the value
  // is lazily computed by computeFunc(), it ensures that that function has been called.
  void ensureHostBufferPopulated();

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
  // In internally, these indexed views are cached. It is safe to call this function many times, after the first the
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
  uint32_t sizeX = 1;
  uint32_t sizeY = 1;
  uint32_t sizeZ = 1;


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

// == Manage a global store of all registered managed buffers


// Get a reference to any buffer that currently exists in Polyscope, by name
// (this one fetches buffers associated with structures)
template <typename T>
ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string bufferName);

// Get a reference to any buffer that currently exists in Polyscope, by name
// (this one fetches buffers associated with quantities)
template <typename T>
ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string quantityName, std::string bufferName);


// NOTE: a vector is a 'bad' choice, we pay O(n) for finds and removals.
// But it'll probably never matter, and using a map is not easy because sometimes string keys are briefly nonunique when
// a structure with a duplicate name is being added.
template <typename T>
struct ManagedBufferRegistry {
public:
  // the actual store
  std::vector<ManagedBuffer<T>*> allBuffers;

  // helpers
  ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string bufferName);
  ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string quantityName, std::string bufferName);
};


  // Helper to get the global cache for a particular type of persistent value
template <typename T>
ManagedBufferRegistry<T>& getManagedBufferRegistryRef();

// clang-format off
namespace detail {

extern ManagedBufferRegistry<float>        managedBufferRegistry_float;
extern ManagedBufferRegistry<double>       managedBufferRegistry_double;
extern ManagedBufferRegistry<glm::vec2>    managedBufferRegistry_vec2;
extern ManagedBufferRegistry<glm::vec3>    managedBufferRegistry_vec3;
extern ManagedBufferRegistry<glm::vec4>    managedBufferRegistry_vec4;
extern ManagedBufferRegistry<std::array<glm::vec3,2>> managedBufferRegistry_arr2vec3;
extern ManagedBufferRegistry<std::array<glm::vec3,3>> managedBufferRegistry_arr3vec3;
extern ManagedBufferRegistry<std::array<glm::vec3,4>> managedBufferRegistry_arr4vec3;
extern ManagedBufferRegistry<uint32_t>     managedBufferRegistry_uint32;
extern ManagedBufferRegistry<int32_t>      managedBufferRegistry_int32;
extern ManagedBufferRegistry<glm::uvec2>   managedBufferRegistry_uvec2;
extern ManagedBufferRegistry<glm::uvec3>   managedBufferRegistry_uvec3;
extern ManagedBufferRegistry<glm::uvec4>   managedBufferRegistry_uvec4;

}

} // namespace render
} // namespace polyscope

#include "polyscope/render/managed_buffer.ipp"

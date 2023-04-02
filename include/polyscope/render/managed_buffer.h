// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <functional>
#include <vector>

#include "polyscope/render/engine.h"

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

  // Manage a buffer of data which is explicitly set externally.
  ManagedBuffer(const std::string& name, std::vector<T>& data);

  // Manage a buffer of data which gets computed lazily
  ManagedBuffer(const std::string& name, std::vector<T>& data, std::function<void()> computeFunc);

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
  // When `dataGetsComputed = true`, we are in Case B, and computeFunc() must be set to a callback that does the lazy
  // computing.

  bool dataGetsComputed;             // if true, the value gets computed on-demand by calling computeFunc()
  std::function<void()> computeFunc; // (optional) callback which populates the `data` buffer

  // == Members for indexed data

  // == Basic interactions

  // Ensure that the `data` member vector reference is populated with the current values. In the common-case where the
  // user sets data and it never changes, then this function will do nothing. However, if e.g. the value is being
  // updated directly from GPU memory, this will mirror the updates to the cpu-side vector. Also, if the value is lazily
  // computed by computeFunc(), it ensures that that function has been called.
  void ensureHostBufferPopulated();

  // Combines calling ensureHostBufferPopulated() and returning a reference to the `data` member
  std::vector<T>& getPopulatedHostBufferRef();

  // If the contents of `data` are updated, this function MUST be called. It internally handles concerns like reflecting
  // updates to the render buffer.
  void markHostBufferUpdated();

  // Get the value at index `i`. It may be dynamically fetched from either the cpu-side `data` member or the render
  // buffer, depending on where the data currently lives.
  // If the data lives only on the device-side render buffer, this function is expensive, so don't call it in a loop.
  T getValue(size_t ind);

  // If computeFunc() has already been called to populate the stored data, call it again to recompute the data, and
  // re-fill the buffer if necessary. This function is only meaningful in the case where `dataGetsComputed = true`.
  void recomputeIfPopulated();

  bool hasData(); // true if there is valid data on either the host or device
  size_t size();  // size of the data (number of entries)

  // == Direct access to the GPU (device-side) render buffer

  // NOTE: This class follows the policy that once the render buffer is allocated, it is always immediately kept updated
  // to reflect any external changes.

  // Get a reference to the underlying GPU-side attribute buffer
  // Once this reference is created, it will always be immediately updated to reflect any external changes to the data.
  // (note that if you write to this buffer externally, you MUST call markRenderAttributeBufferUpdated() below)
  std::shared_ptr<render::AttributeBuffer> getRenderAttributeBuffer();

  // Tell Polyscope that you wrote updated data into the render buffer. This MUST be called after externally writing to
  // the buffer from getRenderBuffer() above.
  void markRenderAttributeBufferUpdated();

  // == Indexed views

  // For some data (e.g. values a vertices of a mesh), we store the data in a canonical ordering (one value per vertex),
  // but need to index it at render time according to some specified list of indices (one value per triangle corner).
  // Rendering frameworks _do_ have explicit support for this case (e.g. IndexedArray DrawMode in openGL), but for
  // various reasons it does not make sense to use those features. Instead, we explicitly expand out the data into a
  // render buffer for drawing as view[i] = data[indices[i]].
  //
  // If such an index is used, it should be set to the `indices` argument below, and this class will automatically
  // handle expanding out the indexed data to populate the returned buffer. External callers can still update the data
  // directly on the host, and this class will handle updating an indexed version of the data for drawing.
  //
  // In internally, these indexed views are cached. It is safe to call this function many times, after the first the
  // same view will be returned repeatedly at no additional cost.
  std::shared_ptr<render::AttributeBuffer> getIndexedRenderAttributeBuffer(ManagedBuffer<uint32_t>& indices);

protected:
  // == Internal members

  bool hostBufferIsPopulated; // true if the host buffer contains currently-valid data

  // A mirror of the
  std::shared_ptr<render::AttributeBuffer> renderAttributeBuffer;

  // == Internal representation of indexed views
  // NOTE: this seems like a problem, we are storing pointers as keys in a cache. Here, it works out because if the key
  // ptr becomes invalid, the value weak_ptr must also be invalid, and we check that before dereferencing the key.
  std::vector<std::tuple<render::ManagedBuffer<uint32_t>*, std::weak_ptr<render::AttributeBuffer>>>
      existingIndexedViews;
  void updateIndexedViews();
  void removeDeletedIndexedViews();

  // == Internal helper functions

  void invalidateHostBuffer();

  enum class CanonicalDataSource { HostData = 0, NeedsCompute, RenderBuffer };
  CanonicalDataSource currentCanonicalDataSource();

  // Manage the program which copies indexed data from the renderBuffer to the indexed views
  void ensureHaveBufferIndexCopyProgram();
  void invokeBufferIndexCopyProgram();
  std::shared_ptr<render::ShaderProgram> bufferIndexCopyProgram;
};


} // namespace render
} // namespace polyscope

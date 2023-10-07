// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <memory>
#include <stdexcept>

namespace polyscope {

// These classes are used to track when objects have been deleted. The object being tracked should inherit from
// WeakReferrable, typically at the highest base class. Then, we can get lifetime-tracking weak references to the object
// or its subclasses by calling getWeakHandle();
//
// NOTE: this does _not_ necessarily handle the case where the object is in the midst of being destructed. In that case,
// the time at which the handle reports destruction depends on when the WeakReferrable destructor is called.
//
// (This is somewhat bad design from a C++ standpoint, it would be better to avoid the need for this. But there's lots
// of irregular lifetime management in Polyscope and it's often an easier solution. If only there was a weak_ptr<> for
// unique_ptr<>....)

using WeakHandleDummyType = int32_t;

struct GenericWeakHandle {

  GenericWeakHandle() {}

  GenericWeakHandle(std::shared_ptr<WeakHandleDummyType>& sentinel_, uint64_t uniqueID_)
      : sentinel(std::weak_ptr<WeakHandleDummyType>(sentinel_)), targetUniqueID(uniqueID_) {}

  // Is the object the handle points to still alive?
  bool isValid() const;

  // Clear back to null
  void reset();

  uint64_t getUniqueID() const;

private:
  std::weak_ptr<WeakHandleDummyType> sentinel;
  uint64_t targetUniqueID;
};

template <typename TargetType>
struct WeakHandle : public GenericWeakHandle {

  WeakHandle() {}

  WeakHandle(std::shared_ptr<WeakHandleDummyType> sentinel_, uint64_t uniqueID_, TargetType* targetPtr_)
      : GenericWeakHandle(sentinel_, uniqueID_), targetPtr(targetPtr_) {}

  // Get a reference to the object (only defined if isValid() == true)
  TargetType& get() const { return *targetPtr; };

private:
  TargetType* targetPtr = nullptr;
};

// Classes can extend this class
class WeakReferrable {
public:
  WeakReferrable();
  virtual ~WeakReferrable() = default;

  // Get a handle to the class (this is specific version, which is templated but allows you to get() the actual target
  // type)
  template <typename TargetType>
  WeakHandle<TargetType> getWeakHandle() {
    TargetType* targetPtr = dynamic_cast<TargetType*>(this); // sorry, world
    if (!targetPtr) throw std::runtime_error("[Polyscope] bad getWeakHandle() cast");
    return WeakHandle<TargetType>(weakReferrableDummyRef, weakReferableUniqueID, targetPtr);
  };

  // Get a generic handle to the class (this is general version, which is non-templated does not allow you to get() the
  // actual target type)
  GenericWeakHandle getGenericWeakHandle() { return GenericWeakHandle(weakReferrableDummyRef, weakReferableUniqueID); };

private:
  // paylod is useless, we are just using it to track a shared_ptr
  // which will get destructed when the object does
  std::shared_ptr<WeakHandleDummyType> weakReferrableDummyRef;

  // A unique ID associated with the object instance
  uint64_t weakReferableUniqueID;
};


} // namespace polyscope

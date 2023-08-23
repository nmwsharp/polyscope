// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <memory>

namespace polyscope {

// These classes are used to track when objects have been deleted. The object being tracked should inherit from
// WeakReferrable, typically at the highest base class. Then, we can get lifetime-tracking weak references to the object
// or its subclasses by calling getWeakHandle();
//
// (This is somewhat bad design from a C++ standpoint, it would be better to avoid the need for this. But there's lots
// of irregular lifetime management in Polyscope and it's often an easier solution. If only there was a weak_ptr<> for
// unique_ptr<>....)

using WeakHandleDummyType = int32_t;

template <typename TargetType>
struct WeakHandle {

  WeakHandle(std::shared_ptr<WeakHandleDummyType>& dummyRef, TargetType* targetPtr_)
      : sentinel(std::weak_ptr<WeakHandleDummyType>(dummyRef)), targetPtr(targetPtr_) {}

  // Is the object the handle points to still alive?
  bool isValid() const { return !sentinel.expired(); };

  // Get a reference to the object (only defined if isValid() == true)
  TargetType& get() const { return *targetPtr; };

private:
  std::weak_ptr<WeakHandleDummyType> sentinel;
  TargetType* targetPtr = nullptr;
};

// Classes can extend this class
class WeakReferrable {
public:
  WeakReferrable();
  virtual ~WeakReferrable() = default;

  template <typename TargetType>
  WeakHandle<TargetType> getWeakHandle() {
    TargetType* targetPtr = dynamic_cast<TargetType*>(this); // sorry, world
    if (!targetPtr) throw std::runtime_error("[Polyscope] bad getWeakHandle() cast");
    return WeakHandle<TargetType>{weakReferrableDummyRef, targetPtr};
  };

private:
  // paylod is useless, we are just using it to track a shared_ptr
  // which will get destructed when the object does
  std::shared_ptr<WeakHandleDummyType> weakReferrableDummyRef;
};


} // namespace polyscope

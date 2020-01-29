#pragma once

namespace polyscope {

// Manages a value which is _either_ scaled by `state::lengthScale` (the default), or an absolute value which is not
// scaled.  That is, if `isRelative` is true, then `.get()` will return `val * state::lengthScale`. Otherwise it will
// just return `val`.
//
// Since values are assumed to be relative by default, simplying assigning to a ScaledValue will create a relatve value.

// forward declare
namespace state {
extern double lengthScale;
}

template <typename T>
class ScaledValue {

public:
  // Basic constructors
  ScaledValue() : relativeFlag(true), value() {}
  ScaledValue(T value_, bool relativeFlag_) : relativeFlag(relativeFlag_), value(value_) {}

  // Copy constructor from scaled value of convertible type
  template <typename U>
  ScaledValue(const ScaledValue<U>& otherValue) : relativeFlag(otherValue.relativeFlag), value(otherValue.value) {}

  // Named constructors
  static ScaledValue<T> relative(T value_) { return ScaledValue<T>(value_, true); }
  static ScaledValue<T> absolute(T value_) { return ScaledValue<T>(value_, false); }

  // Explicit getter (returns value in absolute coordinates always, scaling by length if needed)
  T asAbsolute() const { return relativeFlag ? value * state::lengthScale : value; }
  // Get a raw pointer to the underlying parameter (useful for e.g. imgui)
  T* getValuePtr() { return &value; }
  bool isRelative() const { return relativeFlag; }

  // Explicit setters
  void set(T value_, bool relativeFlag_= true) {
    value = value_;
    relativeFlag = relativeFlag_;
  }
  // implicit conversion from scalar creates relative by default
  ScaledValue(const T& relativeValue) : relativeFlag(true), value(relativeValue) {}

  // Make all template variants friends, so conversion can access private members
  template <typename>
  friend class ScaledValue;

private:
  bool relativeFlag;
  T value;
};


// Create an absolute value of the given type
template <typename T>
ScaledValue<T> absoluteValue(T val) {
  return ScaledValue<T>::absolute(val);
}

// Create an absolute value of the given type
template <typename T>
ScaledValue<T> relativeValue(T val) {
  return ScaledValue<T>::relative(val);
}


} // namespace polyscope

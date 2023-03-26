// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/scaled_value.h"
#include "polyscope/types.h"

// So we can cache the types therein
#include "glm/glm.hpp"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/materials.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace polyscope {

// A named variable which "remembers" its previous values via a global cache.
// On construction, the variable checks the cache for a cached value with the same; if one is found it is used instead
// of the construction value. Whenever the value of the variable is modified (or initially created), it is written to
// the cache.
// We carefully overload the copy and move operators to allow the value to take the _value_ of other variables via
// assignment, but always retain its name after creation.

// Note that PersistentValue<T> can only be instantiated if T is one of the types for which the global table is declared
// below and instantiated in persistent_value.cpp.


namespace detail {
// Simple wrapper class holding the cache for persistent values
template <typename T>
class PersistentCache {
public:
  std::unordered_map<std::string, T> cache;
};
// Helper to get the global cache for a particular type of persistent value
template <typename T>
PersistentCache<T>& getPersistentCacheRef();
} // namespace detail

template <typename T>
class PersistentValue {
public:
  // Basic constructor, used on initial creation
  PersistentValue(const std::string& name_, T value_) : name(name_), value(value_) {
    if (detail::getPersistentCacheRef<T>().cache.find(name) != detail::getPersistentCacheRef<T>().cache.end()) {
      value = detail::getPersistentCacheRef<T>().cache[name];
      holdsDefaultValue = false;
    } else {
      // Update cache value
      detail::getPersistentCacheRef<T>().cache[name] = value;
    }
  }

  // Ensure in cache on deletion (see not above reference conversion)
  ~PersistentValue() { set(value); }

  // Don't want copy or move constructors, only operators
  PersistentValue(const PersistentValue&) = delete;
  PersistentValue(const PersistentValue&&) = delete;

  // Copy/move _operators_, which keep the same name but update the value.
  template <typename U>
  PersistentValue<T>& operator=(const PersistentValue<U>& other) {
    set(other.value);
    return *this;
  }
  template <typename U>
  PersistentValue<T>& operator=(const PersistentValue<U>&& other) {
    set(other.value);
    return *this;
  }
  template <typename U>
  PersistentValue<T>& operator=(const U& value_) {
    set(value_);
    return *this;
  }
  template <typename U>
  PersistentValue<T>& operator=(const U&& value_) {
    set(value_);
    return *this;
  }

  // NOTE if you write via this reference, the value will not _actually_ be cached until destruction or
  // manuallyChanged() is called, rather than immediately (ugly, but seems necessary to use with imgui)...
  T& get() { return value; }
  void manuallyChanged() { set(value); }

  // Explicit setter, which takes care of storing in cache
  void set(T value_) {
    value = value_;
    detail::getPersistentCacheRef<T>().cache[name] = value;
    holdsDefaultValue = false;
  }

  // Passive setter, will change value without marking in cache; does nothing if some value has already been directly
  // set (equivalent to constructing with a different value).
  void setPassive(T value_) {
    if (holdsDefaultValue) {
      value = value_;
      detail::getPersistentCacheRef<T>().cache[name] = value;
    }
  }

  // Make all template variants friends, so conversion can access private members
  template <typename>
  friend class PersistentValue;

private:
  const std::string name;
  T value;
  bool holdsDefaultValue = true; // True if the value was set on construction and never changed. False if it was pulled
                                 // from cache or has ever been explicitly set
};

// clang-format off
namespace detail {
extern PersistentCache<double> persistentCache_double;
extern PersistentCache<float> persistentCache_float;
extern PersistentCache<bool> persistentCache_bool;
extern PersistentCache<std::string> persistentCache_string;
extern PersistentCache<glm::vec3> persistentCache_glmvec3;
extern PersistentCache<glm::mat4> persistentCache_glmmat4;
extern PersistentCache<ScaledValue<double>> persistentCache_scaleddouble;
extern PersistentCache<ScaledValue<float>> persistentCache_scaledfloat;
extern PersistentCache<std::vector<std::string>> persistentCache_vectorstring;
extern PersistentCache<ParamVizStyle> persistentCache_paramVizStyle;
extern PersistentCache<BackFacePolicy> persistentCache_BackFacePolicy;
extern PersistentCache<MeshShadeStyle> persistentCache_MeshNormalType;

template<> inline PersistentCache<double>&                   getPersistentCacheRef<double>()                   { return persistentCache_double; }
template<> inline PersistentCache<float>&                    getPersistentCacheRef<float>()                    { return persistentCache_float; }
template<> inline PersistentCache<bool>&                     getPersistentCacheRef<bool>()                     { return persistentCache_bool; }
template<> inline PersistentCache<std::string>&              getPersistentCacheRef<std::string>()              { return persistentCache_string; }
template<> inline PersistentCache<glm::vec3>&                getPersistentCacheRef<glm::vec3>()                { return persistentCache_glmvec3; }
template<> inline PersistentCache<glm::mat4>&                getPersistentCacheRef<glm::mat4>()                { return persistentCache_glmmat4; }
template<> inline PersistentCache<ScaledValue<double>>&      getPersistentCacheRef<ScaledValue<double>>()      { return persistentCache_scaleddouble; }
template<> inline PersistentCache<ScaledValue<float>>&       getPersistentCacheRef<ScaledValue<float>>()       { return persistentCache_scaledfloat; }
template<> inline PersistentCache<std::vector<std::string>>& getPersistentCacheRef<std::vector<std::string>>() { return persistentCache_vectorstring; }
template<> inline PersistentCache<ParamVizStyle>&            getPersistentCacheRef<ParamVizStyle>()            { return persistentCache_paramVizStyle; }
template<> inline PersistentCache<BackFacePolicy>&           getPersistentCacheRef<BackFacePolicy>()           { return persistentCache_BackFacePolicy; }
template<> inline PersistentCache<MeshShadeStyle>&           getPersistentCacheRef<MeshShadeStyle>()           { return persistentCache_MeshNormalType; }
}
// clang-format on


} // namespace polyscope

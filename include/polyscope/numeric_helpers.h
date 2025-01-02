// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <type_traits>

namespace polyscope {


// Base case: call the scalar version
template <typename T>
bool allComponentsFinite(const T& x) {
  return std::isfinite(x);
}

template <>
inline bool allComponentsFinite<glm::vec2>(const glm::vec2& x) {
  return glm::all(glm::isfinite(x));
}
template <>
inline bool allComponentsFinite<glm::vec3>(const glm::vec3& x) {
  return glm::all(glm::isfinite(x));
}
template <>
inline bool allComponentsFinite<glm::vec4>(const glm::vec4& x) {
  return glm::all(glm::isfinite(x));
}

template <>
inline bool allComponentsFinite<glm::uvec2>(const glm::uvec2& x) {
  return true;
}
template <>
inline bool allComponentsFinite<glm::uvec3>(const glm::uvec3& x) {
  return true;
}
template <>
inline bool allComponentsFinite<glm::uvec4>(const glm::uvec4& x) {
  return true;
}

template <>
inline bool allComponentsFinite<glm::mat2x2>(const glm::mat2x2& x) {
  for (size_t i = 0; i < 2; i++)
    if (!allComponentsFinite(glm::row(x, i))) return false;
  return true;
}
template <>
inline bool allComponentsFinite<glm::mat3x3>(const glm::mat3x3& x) {
  for (size_t i = 0; i < 3; i++)
    if (!allComponentsFinite(glm::row(x, i))) return false;
  return true;
}
template <>
inline bool allComponentsFinite<glm::mat4x4>(const glm::mat4x4& x) {
  for (size_t i = 0; i < 4; i++)
    if (!allComponentsFinite(glm::row(x, i))) return false;
  return true;
}

template <>
inline bool allComponentsFinite<std::array<glm::vec3, 2>>(const std::array<glm::vec3, 2>& x) {
  for (size_t i = 0; i < x.size(); i++)
    if (!glm::all(glm::isfinite(x[i]))) return false;
  return true;
}
template <>
inline bool allComponentsFinite<std::array<glm::vec3, 3>>(const std::array<glm::vec3, 3>& x) {
  for (size_t i = 0; i < x.size(); i++)
    if (!glm::all(glm::isfinite(x[i]))) return false;
  return true;
}
template <>
inline bool allComponentsFinite<std::array<glm::vec3, 4>>(const std::array<glm::vec3, 4>& x) {
  for (size_t i = 0; i < x.size(); i++)
    if (!glm::all(glm::isfinite(x[i]))) return false;
  return true;
}


} // namespace polyscope
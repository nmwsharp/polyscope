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
  // handle all other scalar types by converting to float, it's what we'll do anyway
  return std::isfinite(static_cast<float>(x));
}

// avoid double-to-float rounding infs from the line above
template <>
inline bool allComponentsFinite<double>(const double& x) {
  return true;
}

// if we fall through to std::isfinite() for integers, we get compile errors on windows
// due to a name collision with a windows-specific header
// https://github.com/nmwsharp/polyscope/issues/323
template <>
inline bool allComponentsFinite<uint8_t>(const uint8_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<int8_t>(const int8_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<uint16_t>(const uint16_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<int16_t>(const int16_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<uint32_t>(const uint32_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<int32_t>(const int32_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<uint64_t>(const uint64_t& x) {
  return true;
}
template <>
inline bool allComponentsFinite<int64_t>(const int64_t& x) {
  return true;
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
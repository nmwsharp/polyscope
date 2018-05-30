#pragma once

#include <string>

#include <glm/glm.hpp>


namespace polyscope {

// === Memory management
template <typename T>
void safeDelete(T*& x) {
  if (x != nullptr) {
    delete x;
    x = nullptr;
  }
}

template <typename T>
void safeDeleteArray(T*& x) {
  if (x != nullptr) {
    delete[] x;
    x = nullptr;
  }
}


// === String related utilities

// Attempt to get a user-friendly name for a file from its base name
std::string guessNiceNameFromPath(std::string fullname);

// Print large integers in a user-friendly way (like "37.5B")
std::string prettyPrintCount(size_t count);


// === GLM vector operations
inline glm::vec3 componentwiseMin(glm::vec3& vA, glm::vec3& vB) {
  return glm::vec3{std::min(vA.x, vB.x), std::min(vA.y, vB.y), std::min(vA.z, vB.z)};
}
inline glm::vec3 componentwiseMax(glm::vec3& vA, glm::vec3& vB) {
  return glm::vec3{std::max(vA.x, vB.x), std::max(vA.y, vB.y), std::max(vA.z, vB.z)};
}
inline bool isFinite(glm::vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

// === Index management
const size_t INVALID_IND = std::numeric_limits<size_t>::max();

} // namespace polyscope

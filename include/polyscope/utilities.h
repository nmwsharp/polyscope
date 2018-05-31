#pragma once

#include <string>
#include <sstream>
#include <random>

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
inline glm::vec3 componentwiseMin(const glm::vec3& vA, const glm::vec3& vB) {
  return glm::vec3{std::min(vA.x, vB.x), std::min(vA.y, vB.y), std::min(vA.z, vB.z)};
}
inline glm::vec3 componentwiseMax(const glm::vec3& vA, const glm::vec3& vB) {
  return glm::vec3{std::max(vA.x, vB.x), std::max(vA.y, vB.y), std::max(vA.z, vB.z)};
}
inline bool isFinite(glm::vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

inline std::ostream &operator<<(std::ostream &output, const glm::vec3 &v) {
  output << "<" << v.x << ", " << v.y << ", " << v.z << ">";
  return output;
}

inline std::string to_string(const glm::vec3 &v) {
  std::stringstream buffer;
  buffer << v;
  return buffer.str();
}

// === Index management
const size_t INVALID_IND = std::numeric_limits<size_t>::max();


// === Random number generation
extern std::random_device util_random_device;
extern std::mt19937 util_mersenne_twister;

inline double randomUnit() {
  std::uniform_real_distribution<double> dist(0., 1.);
  return dist(util_mersenne_twister);
}

inline double randomReal(double minVal, double maxVal) {
  std::uniform_real_distribution<double> dist(minVal, maxVal);
  return dist(util_mersenne_twister);
}

// Generate a random int in the INCLUSIVE range [lower,upper]
inline int randomInt(int lower, int upper) {
  std::uniform_int_distribution<int> dist(lower, upper);
  return dist(util_mersenne_twister);
}
// Generate a random size_t in the range [0, N)
inline size_t randomIndex(size_t size) {
  std::uniform_int_distribution<size_t> dist(0, size-1);
  return dist(util_mersenne_twister);
}

inline double randomNormal(double mean=0.0, double stddev=1.0) {
  std::normal_distribution<double> dist{mean, stddev};
  return dist(util_mersenne_twister);
}

} // namespace polyscope

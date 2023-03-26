// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <algorithm>
#include <complex>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <tuple>


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

// Ensure that a string satisjies polyscope requirements for structure and quantity names.
// Raises an error on failure.
void validateName(const std::string& name);

// Print large integers in a user-friendly way (like "37.5B")
std::string prettyPrintCount(size_t count);

// Printf to a std::string
template <typename... Args>
std::string str_printf(const std::string& format, Args... args) {
  size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

// Splits e.g. "file.png" to "file" and ".png"
// Very naive, shouldn't be assumed to work for general paths
std::tuple<std::string, std::string> splitExt(std::string f);

// === GLM vector operations
inline glm::vec3 componentwiseMin(const glm::vec3& vA, const glm::vec3& vB) {
  return glm::vec3{std::min(vA.x, vB.x), std::min(vA.y, vB.y), std::min(vA.z, vB.z)};
}
inline glm::vec3 componentwiseMax(const glm::vec3& vA, const glm::vec3& vB) {
  return glm::vec3{std::max(vA.x, vB.x), std::max(vA.y, vB.y), std::max(vA.z, vB.z)};
}
inline glm::vec3 circularPermuteEntries(const glm::vec3& v) {
  // (could be prettier with swizzel)
  return glm::vec3{v.z, v.x, v.y};
}

// Transformation utilities
void splitTransform(const glm::mat4& trans, glm::mat3x4& R, glm::vec3& T);
glm::mat4 buildTransform(const glm::mat3x4& R, const glm::vec3& T);

inline bool isFinite(glm::vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

inline std::ostream& operator<<(std::ostream& output, const glm::vec2& v) {
  output << std::setprecision(std::numeric_limits<float>::max_digits10);
  output << "<" << v.x << ", " << v.y << ">";
  return output;
}
inline std::ostream& operator<<(std::ostream& output, const glm::vec3& v) {
  output << std::setprecision(std::numeric_limits<float>::max_digits10);
  output << "<" << v.x << ", " << v.y << ", " << v.z << ">";
  return output;
}
inline std::ostream& operator<<(std::ostream& output, const glm::vec4& v) {
  output << std::setprecision(std::numeric_limits<float>::max_digits10);
  output << "<" << v.x << ", " << v.y << ", " << v.z << "," << v.w << ">";
  return output;
}
inline std::string to_string(const glm::vec3& v) {
  std::stringstream buffer;
  buffer << v;
  return buffer.str();
}
inline std::string to_string_short(const glm::vec3& v) { return str_printf("<%1.3f, %1.3f, %1.3f>", v[0], v[1], v[2]); }

// === Index management
const size_t INVALID_IND = std::numeric_limits<size_t>::max();
const uint32_t INVALID_IND_32 = std::numeric_limits<uint32_t>::max();
const uint64_t INVALID_IND_64 = std::numeric_limits<uint64_t>::max();

template <typename T>
std::vector<T> applyPermutation(const std::vector<T>& input, const std::vector<size_t>& perm) {
  // TODO figure out if there's a copy to be avoided here
  if (perm.size() == 0) {
    return input;
  }
  std::vector<T> result(perm.size());
  for (size_t i = 0; i < perm.size(); i++) {
    result[i] = input[perm[i]];
  }
  return result;
}

// Same as applyPermutation() above, but on 32-bit indices, and we gave it a more accurate name
template <typename T>
std::vector<T> gather(const std::vector<T>& input, const std::vector<uint32_t>& perm) {
  // TODO figure out if there's a copy to be avoided here
  if (perm.size() == 0) {
    return input;
  }
  std::vector<T> result(perm.size());
  for (size_t i = 0; i < perm.size(); i++) {
    result[i] = input[perm[i]];
  }
  return result;
}


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
  std::uniform_int_distribution<size_t> dist(0, size - 1);
  return dist(util_mersenne_twister);
}

inline double randomNormal(double mean = 0.0, double stddev = 1.0) {
  std::normal_distribution<double> dist{mean, stddev};
  return dist(util_mersenne_twister);
}

// === ImGui utilities

// Displays a little helper icon which shows the text on hover
void ImGuiHelperMarker(const char* text);

// === Math utilities
const double PI = 3.14159265358979323;

typedef std::complex<double> Complex;


} // namespace polyscope

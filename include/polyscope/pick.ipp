// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {
namespace pick {

// Constant for bit-bashing functions below
// single-precision floats always have at least 22 bits of integer mantissa, and 22*3 > 64, so we can safely store 64
// bit integer quantities like size_t usually is in a vec3
const uint64_t bitsForPickPacking = 22;
// const int bitsForPickPacking = 7; // useful for testing, makes pick coloring visually distingushable

inline glm::vec3 indToVec(size_t globalInd) {

  // NOTE: there is a duplicate version of this logic in a macro below, which must be kept in sync.

  // Can comfortably fit a 22 bit integer exactly in a single precision float
  uint64_t factor = 1 << bitsForPickPacking;
  uint64_t mask = factor - 1;
  double factorF = factor;

  uint64_t low = globalInd & mask;
  globalInd = globalInd >> bitsForPickPacking;
  uint64_t med = globalInd & mask;
  globalInd = globalInd >> bitsForPickPacking;
  uint64_t high = globalInd;

  return glm::vec3{static_cast<double>(low) / factorF, static_cast<double>(med) / factorF,
                   static_cast<double>(high) / factorF};
}
inline uint64_t vecToInd(glm::vec3 vec) {

  uint64_t factor = 1 << bitsForPickPacking;
  double factorF = factor;

  uint64_t low = static_cast<uint64_t>(factorF * vec.x);
  uint64_t med = static_cast<uint64_t>(factorF * vec.y);
  uint64_t high = static_cast<uint64_t>(factorF * vec.z);

  // Debug check
  if (low != (factorF * vec.x) || med != (factorF * vec.y) || high != (factorF * vec.z)) {
    // throw std::logic_error("Float to index conversion failed, bad value in float.");
    // occasionally we get weird data back in unusually cases like clicking right on border or multiple monitors...
    // maybe one day we can debug it.
    return 0;
  }

  uint64_t ind = (high << (2 * bitsForPickPacking)) + (med << bitsForPickPacking) + low;
  return ind;
}


// == Weird alternate implementation of indToVec()
// This is here because sometimes we want to evaluate the indToVec() bit-bashing logic in a shader, and get exactly
// the same result as the C++ version above. A GLSL implementation is not too bad, but it's hard to test to ensure
// it really matches.
//
// The solution here is a funky macro'd implementation, that compiles as C++ or GLSL. We compile it as C++ in the tests
// and verify it matches the usual indToVec() implementation, then compile it as GLSL in shaders.
//
// All of the macro stuff you see below is just boilerplate to make that possible.

#define POLYSCOPE_PICK_STR_H(...) #__VA_ARGS__
#define POLYSCOPE_PICK_STR(...) POLYSCOPE_PICK_STR_H(__VA_ARGS__)

// See note above. This is logic that is meant to be identical to indToVec().
// clang-format off
#define POLYSCOPE_PICK_INDEX_COLOR_BODY                                                    \
  POLYSCOPE_PICK_UINT idxLow  = pickStartLow + primID;                                     \
  POLYSCOPE_PICK_UINT carry   = (idxLow < pickStartLow) ? 1u : 0u;                         \
  POLYSCOPE_PICK_UINT idxHigh = pickStartHigh + carry;                                     \
  POLYSCOPE_PICK_UINT low22  = idxLow & 0x3FFFFFu;                                         \
  POLYSCOPE_PICK_UINT med22  = ((idxLow >> 22u) | (idxHigh << 10u)) & 0x3FFFFFu;           \
  POLYSCOPE_PICK_UINT high22 = (idxHigh >> 12u) & 0x3FFFFFu;                               \
  return POLYSCOPE_PICK_VEC3(float(low22), float(med22), float(high22)) / 4194304.0f;
// clang-format on

// C++ version: compile the body with C++ types.
#define POLYSCOPE_PICK_UINT uint32_t
#define POLYSCOPE_PICK_VEC3 glm::vec3
inline glm::vec3 pickIndexToColorImpl(uint32_t pickStartLow, uint32_t pickStartHigh, uint32_t primID) {
  POLYSCOPE_PICK_INDEX_COLOR_BODY
}
#undef POLYSCOPE_PICK_UINT
#undef POLYSCOPE_PICK_VEC3

// Convenience wrapper for C++ callers that have a combined uint64_t pickStart.
inline glm::vec3 pickIndexToColor(uint64_t pickStart, uint32_t primID) {
  return pickIndexToColorImpl(static_cast<uint32_t>(pickStart & 0xFFFFFFFFull), static_cast<uint32_t>(pickStart >> 32),
                              primID);
}

// GLSL string macro.  Stringifies POLYSCOPE_PICK_INDEX_COLOR_BODY using GLSL type names.
// REQUIRES: POLYSCOPE_PICK_UINT must equal "uint" and POLYSCOPE_PICK_VEC3 must equal "vec3"
// at the point of expansion (see common.cpp for the usage pattern).
#define POLYSCOPE_PICK_INDEX_TO_COLOR_GLSL                                                                             \
  "vec3 pickIndexToColor(uint pickStartLow, uint pickStartHigh, uint primID) { " POLYSCOPE_PICK_STR(                   \
      POLYSCOPE_PICK_INDEX_COLOR_BODY) " }"

} // namespace pick
} // namespace polyscope

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

} // namespace pick
} // namespace polyscope

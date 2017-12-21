#pragma once

#include "polyscope/structure.h"

#include "geometrycentral/vector3.h"

#include <cstdint>

namespace polyscope {
namespace pick {


// Request 'count' contiguous indices for drawing a pick buffer. The return value is the start of the range.
size_t requestPickBufferRange(Structure* requestingStructure, size_t count);


// Get the currently pick index, and the structure that index is mapped to.
// Index is returned in "local" indices for that structure, such that '0' corresponds to the start of the range that was
// retned by requestPickBufferRange().
// If nothing is selected, returns nullptr and max_int.
Structure* getCurrentPickElement(uint32_t& localInd);

// Clear out picking related data
void resetPick();


// Constant for bit-bashing functions below
// const int bitsForPickPacking = 22;
const int bitsForPickPacking = 4; // useful for testing

// Convert indices to color and back
inline geometrycentral::Vector3 indToVec(size_t ind) {

  // Can comfortably fit a 22 bit integer exactly in a single precision float
  size_t factor = 1 << bitsForPickPacking;
  size_t mask = factor - 1;
  double factorF = factor;

  size_t low = ind & mask;
  ind = ind >> bitsForPickPacking;
  size_t med = ind & mask;
  ind = ind >> bitsForPickPacking;
  size_t high = ind;

  std::cout << "low = " << low << " med = " << med << " high = " << high << std::endl;

  return geometrycentral::Vector3{static_cast<double>(low) / factorF, static_cast<double>(med) / factorF,
                                  static_cast<double>(high) / factorF};
  //   return geometrycentral::Vector3{static_cast<double>(low) / factorF, static_cast<double>(med) / factorF,
  //                                   static_cast<double>(133) / factorF};
}
inline size_t vecToInd(geometrycentral::Vector3 vec) {

  size_t factor = 1 << bitsForPickPacking;
  double factorF = factor;

  size_t low = static_cast<size_t>(factorF * vec.x);
  size_t med = static_cast<size_t>(factorF * vec.y);
  size_t high = static_cast<size_t>(factorF * vec.z);

  std::cout << "lowF = " << (factorF * vec.x) << " medF = " << (factorF * vec.y) << " highF = " << (factorF * vec.z)
            << std::endl;
  printf("vecToInd: %20.15lf %20.15lf %20.15lf\n", (factorF * vec.x), (factorF * vec.y), (factorF * vec.z));
  std::cout << "low = " << low << " med = " << med << " high = " << high << std::endl;

  // Debug check
  if (low != (factorF * vec.x) || med != (factorF * vec.y) || high != (factorF * vec.z)) {
    throw std::logic_error("Float to index conversion failed, bad value in float.");
  }

  size_t ind = (high << (2*bitsForPickPacking)) + (med << bitsForPickPacking) + low;
  return ind;
}


// The currently selected index from the pick buffer, if one is selected.
extern size_t currPickInd;
extern bool haveSelection;
extern bool pickWasDoubleClick;

} // namespace pick
} // namespace polyscope

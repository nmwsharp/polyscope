// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/structure.h"

#include <cstdint>

namespace polyscope {
namespace pick {


// The currently selected index from the pick buffer, if one is selected.
extern size_t currPickInd;
extern bool haveSelection;
extern bool alwaysEvaluatePick;
extern bool pickIsFromThisFrame;
extern bool pickWasDoubleClick; // note: structures may act on this and set it to false afterwards to ensure action only
                                // happens once

// Request 'count' contiguous indices for drawing a pick buffer. The return value is the start of the range.
size_t requestPickBufferRange(Structure* requestingStructure, size_t count);

// Get the currently pick index, and the structure that index is mapped to.
// Index is returned in "local" indices for that structure, such that '0' corresponds to the start of the range that was
// returned by requestPickBufferRange().
// If nothing is selected, returns nullptr and max_int.
Structure* getCurrentPickElement(size_t& localInd);

void evaluatePickQuery(int xPos, int yPos);

void setCurrentPickElement(size_t pickInd, bool wasDoubleClick);

// If something from this structure is selected, clear the selection (useful if a structure is being deleted)
void clearPickIfStructureSelected(Structure* s);

// Clear out picking related data
void resetPick();

// Constant for bit-bashing functions below
// single-precision floats always have at least 22 bits of integer mantissa, and 22*3 > 64, so we can safely store 64
// bit integer quantities like size_t usually is in a vec3
const int bitsForPickPacking = 22;
// const int bitsForPickPacking = 7; // useful for testing, makes pick coloring visually distingushable

// Convert indices to color and back
inline glm::vec3 indToVec(size_t ind) {

  // Can comfortably fit a 22 bit integer exactly in a single precision float
  size_t factor = 1 << bitsForPickPacking;
  size_t mask = factor - 1;
  double factorF = factor;

  size_t low = ind & mask;
  ind = ind >> bitsForPickPacking;
  size_t med = ind & mask;
  ind = ind >> bitsForPickPacking;
  size_t high = ind;

  return glm::vec3{static_cast<double>(low) / factorF, static_cast<double>(med) / factorF,
                   static_cast<double>(high) / factorF};
}
inline size_t vecToInd(glm::vec3 vec) {

  size_t factor = 1 << bitsForPickPacking;
  double factorF = factor;

  size_t low = static_cast<size_t>(factorF * vec.x);
  size_t med = static_cast<size_t>(factorF * vec.y);
  size_t high = static_cast<size_t>(factorF * vec.z);

  // Debug check
  if (low != (factorF * vec.x) || med != (factorF * vec.y) || high != (factorF * vec.z)) {
    // throw std::logic_error("Float to index conversion failed, bad value in float.");
    // occasionally we get weird data back in unusually cases like clicking right on border or multiple monitors...
    // maybe one day we can debug it.
    return 0;
  }

  size_t ind = (high << (2 * bitsForPickPacking)) + (med << bitsForPickPacking) + low;
  return ind;
}

} // namespace pick
} // namespace polyscope

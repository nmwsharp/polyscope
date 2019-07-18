// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/pick.h"

#include "polyscope/polyscope.h"

#include <limits>
#include <tuple>

using std::cout;
using std::endl;

namespace polyscope {
namespace pick {

size_t currLocalPickInd = 0;
Structure* currPickStructure = nullptr;
bool haveSelection = false;
bool pickWasDoubleClick = false;
bool alwaysEvaluatePick = false;
bool pickIsFromThisFrame = false;

// The next pick index that a structure can use to identify its elements
// (get it by calling request pickBufferRange())
size_t nextPickBufferInd = 1; // 0 reserved for "none"
std::vector<std::tuple<size_t, size_t, Structure*>> structureRanges;

void resetPick() {
  haveSelection = false;
  pickWasDoubleClick = false;
  currLocalPickInd = 0;
  currPickStructure = nullptr;
  pickIsFromThisFrame = false;
}

void clearPickIfStructureSelected(Structure* s) {
  if (haveSelection && currPickStructure == s) {
    resetPick();
  }
}

Structure* getCurrentPickElement(size_t& localInd) {

  // Check if anything is selected at all
  if (!haveSelection) {
    localInd = std::numeric_limits<size_t>::max();
    return nullptr;
  }

  localInd = currLocalPickInd;
  return currPickStructure;
}

void setCurrentPickElement(size_t newPickInd, bool wasDoubleClick) {

  pickIsFromThisFrame = true;

  // Loop through the ranges that we have allocated to find the one correpsonding to this structure.
  for (const auto& x : structureRanges) {

    size_t rangeStart = std::get<0>(x);
    size_t rangeEnd = std::get<1>(x);
    Structure* structure = std::get<2>(x);

    if (newPickInd >= rangeStart && newPickInd < rangeEnd) {
      currLocalPickInd = newPickInd - rangeStart;
      currPickStructure = structure;
      haveSelection = true;
      pickWasDoubleClick = wasDoubleClick;
      return;
    }
  }

  error("Pick index " + std::to_string(newPickInd) + " does not correspond to any allocated range.");
  currLocalPickInd = std::numeric_limits<size_t>::max();
  currPickStructure = nullptr;
  return;
}


size_t requestPickBufferRange(Structure* requestingStructure, size_t count) {

  // Check if we can satisfy the request
  size_t maxPickInd = std::numeric_limits<size_t>::max();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
  if (bitsForPickPacking < 22) {
    maxPickInd = 1ULL << (bitsForPickPacking * 3);
  }
#pragma GCC diagnostic pop


  if (count > maxPickInd || maxPickInd - count < nextPickBufferInd) {
    error("Wow, you sure do have a lot of stuff, I can't even count it all. (Ran out of indices while enumerating "
          "structure elements for pick buffer.)");
  }

  size_t ret = nextPickBufferInd;
  nextPickBufferInd += count;
  structureRanges.push_back(std::make_tuple(ret, nextPickBufferInd, requestingStructure));
  return ret;
}

} // namespace pick
} // namespace polyscope

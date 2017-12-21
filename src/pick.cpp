#include "polyscope/pick.h"

#include "polyscope/polyscope.h"

#include <limits>
#include <tuple>

namespace polyscope {
namespace pick {

size_t currPickInd = 0;
bool haveSelection = false;
bool pickWasDoubleClick = false;

// The next pick index that a structure can use to identify its elements
// (get it by calling request pickBufferRange())
size_t nextPickBufferInd = 1; // 0 reserved for "none"
std::vector<std::tuple<size_t , size_t, Structure*>> structureRanges;

void resetPick() {
  haveSelection = false;
  pickWasDoubleClick = false;
  currPickInd = 0;
}

Structure* getCurrentPickElement(size_t & localInd) {

  // Check if anything is selected at all
  if (!haveSelection) {
    localInd = std::numeric_limits<size_t >::max();
    return nullptr;
  }

  // Loop through the ranges that we have allocated to find the one correpsonding to this structure.
  for (const auto& x : structureRanges) {

    size_t rangeStart = std::get<0>(x);
    size_t rangeEnd = std::get<1>(x);
    Structure* structure = std::get<2>(x);

    if (currPickInd >= rangeStart && currPickInd < rangeEnd) {
      localInd = currPickInd - rangeStart;
      return structure;
    }
  }

  error("Pick index does not correspond to any allocated range.");
  localInd = std::numeric_limits<size_t >::max();
  return nullptr;
}

/*
size_t requestPickBufferRange(Structure* requestingStructure, size_t count) {

  // Check if we can currently satisfy the request
  size_t maxPickInd = std::numeric_limits<size_t >::max();
  size_t requestedMax = static_cast<size_t>(nextPickBufferInd) + count;
  if (requestedMax < maxPickInd) {
    size_t ret = nextPickBufferInd;
    nextPickBufferInd += count;
    structureRanges.push_back(std::make_tuple(ret, nextPickBufferInd, requestingStructure));
    return ret;
  }

  // If we couldn't satisfy the request, try to re-index all structures to reclaim pick indices
  // Note: We should never run out of indices while trying to do this, because these structures are all already indexed
  // when the method started, so we must have enough indices to satisfy them.
  nextPickBufferInd = 0;
  structureRanges.clear();
  for (auto cat : state::structureCategories) {
    for (auto x : cat.second) {

      if (x.second == requestingStructure) {
        continue;
      }

      x.second->preparePick();
    }
  }

  // Check if we have enough indices to satisfy now
  requestedMax = static_cast<size_t>(nextPickBufferInd) + count;
  if (requestedMax < maxPickInd) {
    size_t ret = nextPickBufferInd;
    nextPickBufferInd += count;
    structureRanges.push_back(std::make_tuple(ret, nextPickBufferInd, requestingStructure));
    return ret;
  }

  // If we don't have enough indices to satify after re-indexing, we never will. Throw an error.
  error("Wow, you sure do have a lot of stuff, I can't even count it all. (Ran out of indices while enumerating "
        "structure elements for pick buffer.)");

  return 0;
}
*/

size_t requestPickBufferRange(Structure* requestingStructure, size_t count) {

  // Check if we can satisfy the request
  size_t maxPickInd = std::numeric_limits<size_t >::max();
  if(bitsForPickPacking < 22) {
    maxPickInd = 1 << (bitsForPickPacking * 3);
  }


  if(count > maxPickInd || maxPickInd - count < nextPickBufferInd) {
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

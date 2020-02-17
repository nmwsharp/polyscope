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
bool haveSelectionVal = false;

// The next pick index that a structure can use to identify its elements
// (get it by calling request pickBufferRange())
size_t nextPickBufferInd = 1; // 0 reserved for "none"
// Track which ranges have been allocated to which structures
std::vector<std::tuple<size_t, size_t, Structure*>> structureRanges;


// == Set up picking
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
    error("Wow, you sure do have a lot of stuff, Polyscope can't even count it all. (Ran out of indices while "
          "enumerating structure elements for pick buffer.)");
  }

  size_t ret = nextPickBufferInd;
  nextPickBufferInd += count;
  structureRanges.push_back(std::make_tuple(ret, nextPickBufferInd, requestingStructure));
  return ret;
}

// Note: evaluatePickQuery() currently lives in polyscope.cpp, because it touches all the rendering data. See comment
// there.

// == Manage stateful picking

void resetSelection() {
  haveSelectionVal = false;
  currLocalPickInd = 0;
  currPickStructure = nullptr;
}

bool haveSelection() { return haveSelectionVal; }

void resetSelectionIfStructure(Structure* s) {
  if (haveSelectionVal && currPickStructure == s) {
    resetSelection();
  }
}

std::pair<Structure*, size_t> getSelection() {
  if (haveSelectionVal) {
    return {currPickStructure, currLocalPickInd};
  } else {
    return {nullptr, 0};
  }
}

void setSelection(std::pair<Structure*, size_t> newPick) {
  if (newPick.first == nullptr) {
    resetSelection();
  } else {
    haveSelectionVal = true;
    currPickStructure = newPick.first;
    currLocalPickInd = newPick.second;
  }
}

// == Helpers

std::pair<Structure*, size_t> globalIndexToLocal(size_t globalInd) {
  // Loop through the ranges that we have allocated to find the one correpsonding to this structure.
  for (const auto& x : structureRanges) {

    size_t rangeStart = std::get<0>(x);
    size_t rangeEnd = std::get<1>(x);
    Structure* structure = std::get<2>(x);

    if (globalInd >= rangeStart && globalInd < rangeEnd) {
      return {structure, globalInd - rangeStart};
    }
  }

  return {nullptr, 0};
}

size_t localIndexToGlobal(std::pair<Structure*, size_t> localPick) {
  if (localPick.first == nullptr) return 0;

  for (const auto& x : structureRanges) {
    size_t rangeStart = std::get<0>(x);
    size_t rangeEnd = std::get<1>(x);
    Structure* structure = std::get<2>(x);

    if (structure == localPick.first) {
      return rangeStart + localPick.second;
    }
  }

  throw std::runtime_error("structure does not match any allocated pick range");
  return 0;
}


std::pair<Structure*, size_t> evaluatePickQuery(int xPos, int yPos) {

  // Be sure not to pick outside of buffer
  if (xPos < 0 || xPos >= view::bufferWidth || yPos < 0 || yPos >= view::bufferHeight) {
    return {nullptr, 0};
  }

  render::FrameBuffer* pickFramebuffer = render::engine->pickFramebuffer.get();

  pickFramebuffer->resize(view::bufferWidth, view::bufferHeight);
  pickFramebuffer->setViewport(0, 0, view::bufferWidth, view::bufferHeight);
  if (!pickFramebuffer->bindForRendering()) return {nullptr, 0};
  pickFramebuffer->clear();

  // Render pick buffer
  for (auto cat : state::structures) {
    for (auto x : cat.second) {
      x.second->drawPick();
    }
  }

  // Read from the pick buffer
  std::array<float, 4> result = pickFramebuffer->readFloat4(xPos, view::bufferHeight - yPos);
  size_t globalInd = pick::vecToInd(glm::vec3{result[0], result[1], result[2]});

  return pick::globalIndexToLocal(globalInd);
}

} // namespace pick


} // namespace polyscope

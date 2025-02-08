// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/pick.h"

#include "polyscope/polyscope.h"

#include <limits>
#include <tuple>
#include <unordered_map>

namespace polyscope {

PickResult queryPickAtScreenCoords(glm::vec2 screenCoords) {
  int xInd, yInd;
  glm::ivec2 bufferInds = view::screenCoordsToBufferIndsVec(screenCoords);
  return queryPickAtBufferInds(bufferInds);
}
PickResult queryPickAtBufferInds(glm::ivec2 bufferInds) {
  PickResult result;

  // Query the render buffer
  result.position = view::bufferIndsToWorldPosition(bufferInds);
  result.depth = glm::length(result.position - view::getCameraWorldPosition());

  // Query the pick buffer
  std::pair<Structure*, size_t> rawPickResult = pick::pickAtBufferCoords(bufferInds.x, bufferInds.y);

  // Transcribe result into return tuple
  result.structure = rawPickResult.first;
  result.bufferCoords = bufferInds;
  result.screenCoords = view::bufferIndsToScreenCoords(bufferInds);
  if (rawPickResult.first == nullptr) {
    result.isHit = false;
    result.structureType = "";
    result.structureName = "";
    result.localIndex = INVALID_IND_64;
  } else {
    result.structureHandle = result.structure->getWeakHandle<Structure>();
    result.isHit = true;
    std::tuple<std::string, std::string> lookupResult = lookUpStructure(rawPickResult.first);
    result.structureType = std::get<0>(lookupResult);
    result.structureName = std::get<1>(lookupResult);
    result.localIndex = rawPickResult.second;
  }

  return result;
}


namespace pick {

PickResult& currSelectionPickResult = state::globalContext.currSelectionPickResult;
bool& haveSelectionVal = state::globalContext.haveSelectionVal;

// The next pick index that a structure can use to identify its elements
// (get it by calling request pickBufferRange())
uint64_t& nextPickBufferInd = state::globalContext.nextPickBufferInd; // 0 reserved for "none"

// Track which ranges have been allocated to which structures
std::unordered_map<Structure*, std::tuple<uint64_t, uint64_t>> structureRanges = state::globalContext.structureRanges;


// == Set up picking
uint64_t requestPickBufferRange(Structure* requestingStructure, uint64_t count) {

  // Check if we can satisfy the request
  uint64_t maxPickInd = std::numeric_limits<uint64_t>::max();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
  if (bitsForPickPacking < 22) {
    uint64_t bitMax = 1ULL << (bitsForPickPacking * 3);
    if (bitMax < maxPickInd) {
      maxPickInd = bitMax;
    }
  }
#pragma GCC diagnostic pop

  if (count > maxPickInd || maxPickInd - count < nextPickBufferInd) {
    exception("Wow, you sure do have a lot of stuff, Polyscope can't even count it all. (Ran out of indices while "
              "enumerating structure elements for pick buffer.)");
  }

  uint64_t ret = nextPickBufferInd;
  nextPickBufferInd += count;
  structureRanges[requestingStructure] = std::make_tuple(ret, nextPickBufferInd);
  return ret;
}

// == Manage stateful picking

void resetSelection() {
  haveSelectionVal = false;
  currSelectionPickResult = PickResult();
}

bool haveSelection() { return haveSelectionVal; }

void resetSelectionIfStructure(Structure* s) {
  if (haveSelectionVal && currSelectionPickResult.structure == s) {
    resetSelection();
  }
}

PickResult getSelection() { return currSelectionPickResult; }

void setSelection(PickResult newPick) {
  if (!newPick.isHit) {
    resetSelection();
  } else {
    haveSelectionVal = true;
    currSelectionPickResult = newPick;
  }
}

// == Helpers

std::pair<Structure*, uint64_t> globalIndexToLocal(uint64_t globalInd) {

  // ONEDAY: this could be asymptotically better if we cared

  // Loop through the ranges that we have allocated to find the one correpsonding to this structure.
  for (const auto& x : structureRanges) {

    Structure* structure = x.first;
    uint64_t rangeStart = std::get<0>(x.second);
    uint64_t rangeEnd = std::get<1>(x.second);

    if (globalInd >= rangeStart && globalInd < rangeEnd) {
      return {structure, globalInd - rangeStart};
    }
  }

  return {nullptr, 0};
}

uint64_t localIndexToGlobal(std::pair<Structure*, uint64_t> localPick) {
  if (localPick.first == nullptr) return 0;

  if (structureRanges.find(localPick.first) == structureRanges.end()) {
    exception("structure does not match any allocated pick range");
  }

  std::tuple<uint64_t, uint64_t> range = structureRanges[localPick.first];
  uint64_t rangeStart = std::get<0>(range);
  uint64_t rangeEnd = std::get<1>(range);
  return rangeStart + localPick.second;
}

std::pair<Structure*, uint64_t> pickAtScreenCoords(glm::vec2 screenCoords) {
  int xInd, yInd;
  std::tie(xInd, yInd) = view::screenCoordsToBufferInds(screenCoords);
  return pickAtBufferCoords(xInd, yInd);
}

std::pair<Structure*, uint64_t> pickAtBufferCoords(int xPos, int yPos) { return evaluatePickQuery(xPos, yPos); }

std::pair<Structure*, uint64_t> evaluatePickQuery(int xPos, int yPos) {

  // NOTE: hack used for debugging: if xPos == yPos == -1 we do a pick render but do not query the value.

  // Be sure not to pick outside of buffer
  if (xPos < -1 || xPos >= view::bufferWidth || yPos < -1 || yPos >= view::bufferHeight) {
    return {nullptr, 0};
  }

  render::FrameBuffer* pickFramebuffer = render::engine->pickFramebuffer.get();

  render::engine->setDepthMode(DepthMode::Less);
  render::engine->setBlendMode(BlendMode::Disable);

  pickFramebuffer->resize(view::bufferWidth, view::bufferHeight);
  pickFramebuffer->setViewport(0, 0, view::bufferWidth, view::bufferHeight);
  pickFramebuffer->clearColor = glm::vec3{0., 0., 0.};
  if (!pickFramebuffer->bindForRendering()) return {nullptr, 0};
  pickFramebuffer->clear();

  // Render pick buffer
  for (auto& cat : state::structures) {
    for (auto& x : cat.second) {
      x.second->drawPick();
    }
  }

  if (xPos == -1 || yPos == -1) {
    return {nullptr, 0};
  }

  // Read from the pick buffer
  std::array<float, 4> result = pickFramebuffer->readFloat4(xPos, view::bufferHeight - yPos);
  uint64_t globalInd = pick::vecToInd(glm::vec3{result[0], result[1], result[2]});

  return pick::globalIndexToLocal(globalInd);
}

} // namespace pick


} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/pick.h"

#include "polyscope/polyscope.h"

#include <limits>
#include <tuple>
#include <unordered_map>

namespace polyscope {

PickResult pickAtScreenCoords(glm::vec2 screenCoords) {
  int xInd, yInd;
  glm::ivec2 bufferInds = view::screenCoordsToBufferIndsVec(screenCoords);
  return pickAtBufferInds(bufferInds);
}

PickResult pickAtBufferInds(glm::ivec2 bufferInds) {
  PickResult result;

  // Query the pick buffer
  // (this necessarily renders to pickFrameBuffer)
  std::pair<Structure*, size_t> rawPickResult = pick::pickAtBufferCoords(bufferInds.x, bufferInds.y);

  // Query the depth buffer populated above
  render::FrameBuffer* pickFramebuffer = render::engine->pickFramebuffer.get();
  float clipDepth = pickFramebuffer->readDepth(bufferInds.x, view::bufferHeight - bufferInds.y);

  // Transcribe result into return tuple
  result.structure = rawPickResult.first;
  result.bufferInds = bufferInds;
  result.screenCoords = view::bufferIndsToScreenCoords(bufferInds);
  result.position = view::screenCoordsAndDepthToWorldPosition(result.screenCoords, clipDepth);
  result.depth = glm::length(result.position - view::getCameraWorldPosition());

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

// == Manage stateful picking

void resetSelection() {
  state::globalContext.haveSelectionVal = false;
  state::globalContext.currSelectionPickResult = PickResult();
}

bool haveSelection() { return state::globalContext.haveSelectionVal; }

void resetSelectionIfStructure(Structure* s) {
  if (state::globalContext.haveSelectionVal && state::globalContext.currSelectionPickResult.structure == s) {
    resetSelection();
  }
}

PickResult getSelection() { return state::globalContext.currSelectionPickResult; }

void setSelection(PickResult newPick) {
  if (!newPick.isHit) {
    resetSelection();
  } else {
    state::globalContext.haveSelectionVal = true;
    state::globalContext.currSelectionPickResult = newPick;
  }
}


namespace pick {

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

  if (count > maxPickInd || maxPickInd - count < state::globalContext.nextPickBufferInd) {
    exception("Wow, you sure do have a lot of stuff, Polyscope can't even count it all. (Ran out of indices while "
              "enumerating structure elements for pick buffer.)");
  }

  uint64_t ret = state::globalContext.nextPickBufferInd;
  state::globalContext.nextPickBufferInd += count;
  state::globalContext.structureRanges[requestingStructure] =
      std::make_tuple(ret, state::globalContext.nextPickBufferInd);
  return ret;
}

// == Helpers

std::pair<Structure*, uint64_t> globalIndexToLocal(uint64_t globalInd) {

  // ONEDAY: this could be asymptotically better if we cared

  // Loop through the ranges that we have allocated to find the one correpsonding to this structure.
  for (const auto& x : state::globalContext.structureRanges) {

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

  if (state::globalContext.structureRanges.find(localPick.first) == state::globalContext.structureRanges.end()) {
    exception("structure does not match any allocated pick range");
  }

  std::tuple<uint64_t, uint64_t> range = state::globalContext.structureRanges[localPick.first];
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

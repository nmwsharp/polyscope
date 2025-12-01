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
  std::tuple<Structure*, Quantity*, uint64_t> rawPickResult = pick::evaluatePickQueryFull(bufferInds.x, bufferInds.y);

  // Query the depth buffer populated above
  render::FrameBuffer* pickFramebuffer = render::engine->pickFramebuffer.get();
  float clipDepth = pickFramebuffer->readDepth(bufferInds.x, view::bufferHeight - bufferInds.y);

  // Transcribe result into return tuple
  result.structure = std::get<0>(rawPickResult);
  result.quantity = std::get<1>(rawPickResult);
  result.bufferInds = bufferInds;
  result.screenCoords = view::bufferIndsToScreenCoords(bufferInds);
  result.position = view::screenCoordsAndDepthToWorldPosition(result.screenCoords, clipDepth);
  result.depth = glm::length(result.position - view::getCameraWorldPosition());


  // null-initialize, might be overwritten below
  result.isHit = false;
  result.structureType = "";
  result.structureName = "";
  result.quantityName = "";
  result.localIndex = INVALID_IND_64;

  if (result.structure != nullptr) {
    result.structureHandle = result.structure->getWeakHandle<Structure>();
    result.isHit = true;
    result.structureType = result.structure->subtypeName;
    result.structureName = result.structure->name;
    result.localIndex = std::get<2>(rawPickResult);
  }

  if (result.quantity != nullptr) {
    result.isHit = true;
    result.quantityName = result.quantity->name;
    // ideally we would have typenames and WeakHandles for quantities too, but the classes do not offer either of those
    // for now. ONEDAY: add them
    result.localIndex = std::get<2>(rawPickResult);
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
namespace {

// helper sharing logic for both cases below
uint64_t requestPickBufferRange(Structure* requestingStructure, Quantity* requestingQuantity, uint64_t count) {

  if (requestingStructure != nullptr && requestingQuantity != nullptr) {
    // this is an either-or function sharing logic, it shouldn't be called with both non-null
    throw std::logic_error("only one of the requestPickBufferRange() pointers should be null, not both");
  }

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
  if (requestingStructure != nullptr) {
    state::globalContext.structureRanges[requestingStructure] =
        std::make_tuple(ret, state::globalContext.nextPickBufferInd);
  }
  if (requestingQuantity != nullptr) {
    state::globalContext.quantityRanges[requestingQuantity] =
        std::make_tuple(ret, state::globalContext.nextPickBufferInd);
  }
  return ret;
}
} // namespace

uint64_t requestPickBufferRange(Structure* requestingStructure, uint64_t count) {
  return requestPickBufferRange(requestingStructure, nullptr, count);
}

uint64_t requestPickBufferRange(Quantity* requestingQuantity, uint64_t count) {
  return requestPickBufferRange(nullptr, requestingQuantity, count);
}

// == Helpers

std::tuple<Structure*, Quantity*, uint64_t> globalIndexToLocal(uint64_t globalInd) {

  // ONEDAY: this could be asymptotically better if we cared

  // Loop through the ranges that we have allocated to find the one corresponding to this structure.
  for (const auto& x : state::globalContext.structureRanges) {

    Structure* structure = x.first;
    uint64_t rangeStart = std::get<0>(x.second);
    uint64_t rangeEnd = std::get<1>(x.second);

    if (globalInd >= rangeStart && globalInd < rangeEnd) {
      return {structure, nullptr, globalInd - rangeStart};
    }
  }


  // Look through quantity ranges
  for (const auto& x : state::globalContext.quantityRanges) {

    Quantity* quantity = x.first;
    uint64_t rangeStart = std::get<0>(x.second);
    uint64_t rangeEnd = std::get<1>(x.second);

    if (globalInd >= rangeStart && globalInd < rangeEnd) {

      // look up the structure that goes with this quantity
      Structure* structure = &quantity->parent;

      return {structure, quantity, globalInd - rangeStart};
    }
  }

  return {nullptr, nullptr, 0};
}

uint64_t localIndexToGlobal(std::tuple<Structure*, Quantity*, uint64_t> localPick) {
  Structure* structurePtr = std::get<0>(localPick);
  Quantity* quantityPtr = std::get<1>(localPick);
  uint64_t localInd = std::get<2>(localPick);

  if (structurePtr == nullptr && quantityPtr == nullptr) return 0;

  if (state::globalContext.structureRanges.find(structurePtr) != state::globalContext.structureRanges.end()) {
    std::tuple<uint64_t, uint64_t> range = state::globalContext.structureRanges[structurePtr];
    uint64_t rangeStart = std::get<0>(range);
    uint64_t rangeEnd = std::get<1>(range);
    return rangeStart + localInd;
  }

  if (state::globalContext.quantityRanges.find(quantityPtr) != state::globalContext.quantityRanges.end()) {
    std::tuple<uint64_t, uint64_t> range = state::globalContext.quantityRanges[quantityPtr];
    uint64_t rangeStart = std::get<0>(range);
    uint64_t rangeEnd = std::get<1>(range);
    return rangeStart + localInd;
  }

  exception("structure/quantity does not match any allocated pick range");

  return 0;
}

std::pair<Structure*, uint64_t> pickAtScreenCoords(glm::vec2 screenCoords) {
  int xInd, yInd;
  std::tie(xInd, yInd) = view::screenCoordsToBufferInds(screenCoords);
  return pickAtBufferCoords(xInd, yInd);
}

std::pair<Structure*, uint64_t> pickAtBufferCoords(int xPos, int yPos) { return evaluatePickQuery(xPos, yPos); }

std::tuple<Structure*, Quantity*, uint64_t> evaluatePickQueryFull(int xPos, int yPos) {

  // NOTE: hack used for debugging: if xPos == yPos == -1 we do a pick render but do not query the value.

  // Be sure not to pick outside of buffer
  if (xPos < -1 || xPos >= view::bufferWidth || yPos < -1 || yPos >= view::bufferHeight) {
    return {nullptr, nullptr, 0};
  }

  render::FrameBuffer* pickFramebuffer = render::engine->pickFramebuffer.get();

  render::engine->setDepthMode(DepthMode::Less);
  render::engine->setBlendMode(BlendMode::Disable);

  pickFramebuffer->resize(view::bufferWidth, view::bufferHeight);
  pickFramebuffer->setViewport(0, 0, view::bufferWidth, view::bufferHeight);
  pickFramebuffer->clearColor = glm::vec3{0., 0., 0.};
  if (!pickFramebuffer->bindForRendering()) return {nullptr, nullptr, 0};
  pickFramebuffer->clear();

  // Render pick buffer
  for (auto& cat : state::structures) {
    for (auto& x : cat.second) {
      x.second->drawPick();
    }
  }
  for (auto& catMap : state::structures) {
    for (auto& s : catMap.second) {
      s.second->drawPickDelayed();
    }
  }


  if (xPos == -1 || yPos == -1) {
    return {nullptr, nullptr, 0};
  }

  // Read from the pick buffer
  std::array<float, 4> result = pickFramebuffer->readFloat4(xPos, view::bufferHeight - yPos);
  uint64_t globalInd = pick::vecToInd(glm::vec3{result[0], result[1], result[2]});

  return pick::globalIndexToLocal(globalInd);
}

std::pair<Structure*, uint64_t> evaluatePickQuery(int xPos, int yPos) {
  std::tuple<Structure*, Quantity*, uint64_t> t = evaluatePickQueryFull(xPos, yPos);
  return std::pair<Structure*, uint64_t>(std::get<0>(t), std::get<2>(t));
}

} // namespace pick
} // namespace polyscope

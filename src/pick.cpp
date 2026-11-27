// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/pick.h"

#include "polyscope/polyscope.h"

#include <limits>
#include <tuple>
#include <unordered_map>

namespace polyscope {

// The workhorse for all of the pick functions below. Takes buffer inds that are LOCAL to the currently ACTIVE viewport.
// (the public functions below take care of routing to the right viewport and converting coordinates)
static PickResult pickAtActiveViewportBufferInds(glm::ivec2 bufferInds);

PickResult pickAtScreenCoords(glm::vec2 screenCoords) {
  // screenCoords are global, find the viewport they lie in and the local coordinates there
  glm::vec2 localScreenCoords;
  int32_t viewportInd;
  std::tie(localScreenCoords, viewportInd) = view::screenCoordsGlobalToViewport(screenCoords);
  return pickAtScreenCoordsViewport(localScreenCoords, viewportInd);
}

PickResult pickAtBufferInds(glm::ivec2 bufferInds) {
  // bufferInds are global, find the viewport they lie in and the local indices there
  glm::ivec2 localBufferInds;
  int32_t viewportInd;
  std::tie(localBufferInds, viewportInd) = view::bufferIndsGlobalToViewport(bufferInds);
  return pickAtBufferIndsViewport(localBufferInds, viewportInd);
}

PickResult pickAtScreenCoordsViewport(glm::vec2 screenCoords, int32_t viewportInd) {
  int32_t prevViewport = view::getActiveViewportIndex();
  if (viewportInd < 0) viewportInd = prevViewport;
  if (viewportInd != prevViewport) view::activateViewport(viewportInd);
  glm::ivec2 localBufferInds = view::screenCoordsToBufferIndsVecViewport(screenCoords, viewportInd);
  PickResult result = pickAtActiveViewportBufferInds(localBufferInds);
  if (viewportInd != prevViewport) view::activateViewport(prevViewport);
  return result;
}

PickResult pickAtBufferIndsViewport(glm::ivec2 bufferInds, int32_t viewportInd) {
  int32_t prevViewport = view::getActiveViewportIndex();
  if (viewportInd < 0) viewportInd = prevViewport;
  if (viewportInd != prevViewport) view::activateViewport(viewportInd);
  PickResult result = pickAtActiveViewportBufferInds(bufferInds); // already local to the viewport, no conversion
  if (viewportInd != prevViewport) view::activateViewport(prevViewport);
  return result;
}

static PickResult pickAtActiveViewportBufferInds(glm::ivec2 bufferInds) {
  // bufferInds are local to the active viewport
  PickResult result;

  // Render pick buffer and query both color (encoded index) and clip-space depth
  render::PickBufferSample sample = render::engine->renderAndReadPick(bufferInds.x, bufferInds.y, drawStructuresPick);

  std::tuple<Structure*, Quantity*, uint64_t> rawPickResult{nullptr, nullptr, 0};
  float clipDepth = 1.0f;
  if (sample.valid) {
    uint64_t globalInd = pick::vecToInd(glm::vec3{sample.color[0], sample.color[1], sample.color[2]});
    rawPickResult = pick::globalIndexToLocal(globalInd);
    clipDepth = sample.clipDepth;
  }

  // Transcribe result into return tuple
  result.structure = std::get<0>(rawPickResult);
  result.quantity = std::get<1>(rawPickResult);
  result.viewportInd = view::getActiveViewportIndex();
  result.viewportBufferInds = bufferInds;
  result.viewportScreenCoords = view::bufferIndsToScreenCoordsViewport(bufferInds, result.viewportInd);
  result.bufferInds = view::bufferIndsViewportToGlobal(bufferInds, result.viewportInd);
  result.screenCoords = view::bufferIndsToScreenCoords(result.bufferInds);
  result.position =
      view::screenCoordsAndDepthToWorldPositionViewport(result.viewportScreenCoords, clipDepth, result.viewportInd);
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

// === The old, deprecated picking API ===
//
// These predate viewports, so with a single viewport (all that older code could have) they must keep behaving exactly
// as they always have. When using multiple viewports, they follow the same convention as the other viewport-aware
// functions: the coordinates are LOCAL to the viewport `iView`, where iView < 0 means the active viewport. (Some
// internal code relies on this, to pick in whatever viewport it is currently rendering.) The non-deprecated
// polyscope::pickAtScreenCoords() and polyscope::pickAtBufferInds() take global coordinates and find the viewport.

static std::tuple<Structure*, Quantity*, uint64_t> evaluatePickQueryFullInActiveViewport(int xPos, int yPos);

std::pair<Structure*, uint64_t> pickAtScreenCoords(glm::vec2 screenCoords, int32_t iView) {
  glm::ivec2 localInds = view::screenCoordsToBufferIndsVecViewport(screenCoords, iView);
  return evaluatePickQuery(localInds.x, localInds.y, iView);
}

std::pair<Structure*, uint64_t> pickAtBufferCoords(int xPos, int yPos, int32_t iView) {
  return evaluatePickQuery(xPos, yPos, iView);
}

std::tuple<Structure*, Quantity*, uint64_t> evaluatePickQueryFull(int xPos, int yPos, int32_t iView) {
  // the pick is evaluated in the active viewport, so temporarily activate the requested viewport if needed
  int32_t prevViewport = view::getActiveViewportIndex();
  if (iView < 0 || iView == prevViewport) {
    return evaluatePickQueryFullInActiveViewport(xPos, yPos);
  }
  view::activateViewport(iView);
  std::tuple<Structure*, Quantity*, uint64_t> result = evaluatePickQueryFullInActiveViewport(xPos, yPos);
  view::activateViewport(prevViewport);
  return result;
}

static std::tuple<Structure*, Quantity*, uint64_t> evaluatePickQueryFullInActiveViewport(int xPos, int yPos) {

  // NOTE: hack used for debugging: if xPos == yPos == -1 we do a pick render but do not query the value.
  render::PickBufferSample sample = render::engine->renderAndReadPick(xPos, yPos, drawStructuresPick);
  if (!sample.valid) {
    return {nullptr, nullptr, 0};
  }

  uint64_t globalInd = pick::vecToInd(glm::vec3{sample.color[0], sample.color[1], sample.color[2]});
  return pick::globalIndexToLocal(globalInd);
}

std::pair<Structure*, uint64_t> evaluatePickQuery(int xPos, int yPos, int32_t iView) {
  std::tuple<Structure*, Quantity*, uint64_t> t = evaluatePickQueryFull(xPos, yPos, iView);
  return std::pair<Structure*, uint64_t>(std::get<0>(t), std::get<2>(t));
}

} // namespace pick
} // namespace polyscope

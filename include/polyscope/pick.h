// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#include "polyscope/utilities.h"
#include "polyscope/weak_handle.h"

namespace polyscope {

// Forward decls
class Structure;
class Quantity;

// == Main query

// Pick queries test a screen location in the rendered viewport, and return a variety of info about what is underneath
// the pixel at that point, including what structure is under the cursor, and the scene depth and color.
//
// This information can be fed into structure-specific functions like SurfaceMesh::interpretPick(PickResult) to get
// structure-specific info, like which vertex/face was clicked on.

// Return type for pick queries
struct PickResult {
  bool isHit = false;
  Structure* structure = nullptr;
  Quantity* quantity = nullptr;
  WeakHandle<Structure> structureHandle; // same as .structure, but with lifetime tracking
  std::string structureType = "";
  std::string structureName = "";
  std::string quantityName = "";
  glm::vec2 screenCoords;
  glm::ivec2 bufferInds;
  glm::vec3 position;
  float depth;
  uint64_t localIndex = INVALID_IND_64;
};

// Query functions to evaluate a pick.
// Internally, these do a render pass to populate relevant information, then query the resulting buffers.
PickResult pickAtScreenCoords(glm::vec2 screenCoords); // takes screen coordinates
PickResult pickAtBufferInds(glm::ivec2 bufferInds);    // takes indices into render buffer


// == Stateful picking: track and update a current selection

// Get/Set the "selected" item, if there is one
PickResult getSelection();
void setSelection(PickResult newPick);
void resetSelection();
bool haveSelection();
void resetSelectionIfStructure(Structure* s); // If something from this structure is selected, clear the selection
                                              // (useful if a structure is being deleted)

namespace pick {

// Old, deprecated picking API. Use the above functions instead.
// Get the structure which was clicked on (nullptr if none), and the pick ID in local indices for that structure (such
// that 0 is the first index as returned from requestPickBufferRange())
std::pair<Structure*, uint64_t> pickAtScreenCoords(glm::vec2 screenCoords); // takes screen coordinates
std::pair<Structure*, uint64_t> pickAtBufferCoords(int xPos, int yPos);     // takes indices into the buffer
std::pair<Structure*, uint64_t> evaluatePickQuery(int xPos, int yPos); // old, badly named. takes buffer coordinates.
std::tuple<Structure*, Quantity*, uint64_t> evaluatePickQueryFull(int xPos,
                                                                  int yPos); // badly named. takes buffer coordinates.


// == Helpers

// Set up picking (internal)
// Called by a structure/quantity to figure out what data it should render to the pick buffer.
// Request 'count' contiguous indices for drawing a pick buffer. The return value is the start of the range.
uint64_t requestPickBufferRange(Structure* requestingStructure, uint64_t count);
uint64_t requestPickBufferRange(Quantity* requestingQuantity, uint64_t count);

// Convert between global pick indexing for the whole program, and local per-structure pick indexing
std::tuple<Structure*, Quantity*, uint64_t> globalIndexToLocal(uint64_t globalInd);
uint64_t localIndexToGlobal(std::tuple<Structure*, Quantity*, uint64_t> localPick);

// Convert indices to float3 color and back
// Structures will want to use these to fill their pick buffers
inline glm::vec3 indToVec(uint64_t globalInd);
inline uint64_t vecToInd(glm::vec3 vec);

} // namespace pick
} // namespace polyscope

#include "polyscope/pick.ipp"

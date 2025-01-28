// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/structure.h"

#include <cstdint>
#include <utility>

namespace polyscope {

// == Main query

// Pick queries test a screen location in the rendered viewport, and return a variety of info about what is underneath
// the pixel at that point, including what structure is under the cursor, and the scene depth and color.
//
// This information can be fed into structure-specific functions like SurfaceMesh::interpretPick(PickQueryResult) to get
// structure-specific info, like which vertex/face was clicked on.

// Return type for pick queries
struct PickQueryResult {
  bool isHit;
  Structure* structure;
  std::string structureType;
  std::string structureName;
  glm::vec3 position;
  float depth;
};

// Query functions to evaluate a pick.
// Internally, these do a render pass to populate relevant information, then query the resulting buffers.
PickQueryResult queryPickAtScreenCoords(glm::vec2 screenCoords); // takes screen coordinates
PickQueryResult queryPickAtBufferCoords(int xPos, int yPos);     // takes indices into render buffer

namespace pick {

// Old, deprecated picking API. Use the above functions instead.
// Get the structure which was clicked on (nullptr if none), and the pick ID in local indices for that structure (such
// that 0 is the first index as returned from requestPickBufferRange())
std::pair<Structure*, size_t> pickAtScreenCoords(glm::vec2 screenCoords); // takes screen coordinates
std::pair<Structure*, size_t> pickAtBufferCoords(int xPos, int yPos);     // takes indices into the buffer
std::pair<Structure*, size_t> evaluatePickQuery(int xPos, int yPos);      // old, badly named. takes buffer coordinates.

// == Stateful picking: track and update a current selection

// Get/Set the "selected" item, if there is one (output has same meaning as evaluatePickQuery());
std::pair<Structure*, size_t> getSelection();
void setSelection(std::pair<Structure*, size_t> newPick);
void resetSelection();
bool haveSelection();
void resetSelectionIfStructure(Structure* s); // If something from this structure is selected, clear the selection
                                              // (useful if a structure is being deleted)


// == Helpers

// Set up picking (internal)
// Called by a structure to figure out what data it should render to the pick buffer.
// Request 'count' contiguous indices for drawing a pick buffer. The return value is the start of the range.
size_t requestPickBufferRange(Structure* requestingStructure, size_t count);

// Convert between global pick indexing for the whole program, and local per-structure pick indexing
std::pair<Structure*, size_t> globalIndexToLocal(size_t globalInd);
size_t localIndexToGlobal(std::pair<Structure*, size_t> localPick);

// Convert indices to float3 color and back
// Structures will want to use these to fill their pick buffers
inline glm::vec3 indToVec(uint64_t globalInd);
inline uint64_t vecToInd(glm::vec3 vec);

} // namespace pick
} // namespace polyscope

#include "polyscope/pick.ipp"

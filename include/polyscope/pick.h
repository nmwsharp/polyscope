// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/structure.h"

#include <cstdint>
#include <utility>

namespace polyscope {
namespace pick {


// == Set up picking
// Called by a structure to figure out what data it should render to the pick buffer.
// Request 'count' contiguous indices for drawing a pick buffer. The return value is the start of the range.
size_t requestPickBufferRange(Structure* requestingStructure, size_t count);


// == Main query
// Get the structure which was clicked on (nullptr if none), and the pick ID in local indices for that structure (such
// that 0 is the first index as returned from requestPickBufferRange())
std::pair<Structure*, size_t> evaluatePickQuery(int xPos, int yPos);


// == Stateful picking: track and update a current selection

// Get/Set the "selected" item, if there is one (output has same meaning as evaluatePickQuery());
std::pair<Structure*, size_t> getSelection();
void setSelection(std::pair<Structure*, size_t> newPick);
void resetSelection();
bool haveSelection();
void resetSelectionIfStructure(Structure* s); // If something from this structure is selected, clear the selection
                                              // (useful if a structure is being deleted)


// == Helpers

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

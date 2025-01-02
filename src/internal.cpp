// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/internal.h"

#include "polyscope/polyscope.h"

namespace polyscope {
namespace internal {

uint64_t uniqueID = 42;

uint64_t getNextUniqueID() { return uniqueID++; }

bool& pointCloudEfficiencyWarningReported = state::globalContext.pointCloudEfficiencyWarningReported;
FloatingQuantityStructure*& globalFloatingQuantityStructure = state::globalContext.globalFloatingQuantityStructure;

} // namespace internal
} // namespace polyscope

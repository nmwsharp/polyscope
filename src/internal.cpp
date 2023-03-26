// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/internal.h"

namespace polyscope {
namespace internal {

uint64_t uniqueID = 42;

uint64_t getNextUniqueID() { return uniqueID++; }

bool pointCloudEfficiencyWarningReported = false;
FloatingQuantityStructure* globalFloatingQuantityStructure = nullptr;

} // namespace internal
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/internal.h"

#include "polyscope/polyscope.h"

namespace polyscope {
namespace internal {

uint64_t uniqueID = 42;

uint64_t getNextUniqueID() { return uniqueID++; }

bool& pointCloudEfficiencyWarningReported = state::globalContext.pointCloudEfficiencyWarningReported;
FloatingQuantityStructure*& globalFloatingQuantityStructure = state::globalContext.globalFloatingQuantityStructure;

float imguiStackMargin = 10;
float lastWindowHeightPolyscope = 200;
float lastWindowHeightUser = 200;
float lastRightSideFreeX = 10;
float lastRightSideFreeY = 10;
float leftWindowsWidth = -1.;
float rightWindowsWidth = -1.;

} // namespace internal
} // namespace polyscope

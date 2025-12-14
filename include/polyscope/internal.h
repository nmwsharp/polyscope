// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <cstdint>
#include <string>

#include "polyscope/scaled_value.h"
#include "polyscope/types.h"


namespace polyscope {

// forward declaration
class FloatingQuantityStructure;


namespace internal {


// == Various nitty-gritty internal details of Polyscope, which end users certainly should not touch or depend on.

// Get a unique identifier
uint64_t getNextUniqueID();

// track various fire-once warnings
extern bool& pointCloudEfficiencyWarningReported;

// global members
extern FloatingQuantityStructure*& globalFloatingQuantityStructure;


// == UI and layout related
extern float imguiStackMargin;
extern float lastWindowHeightPolyscope;
extern float lastWindowHeightUser;
extern float lastRightSideFreeX;
extern float lastRightSideFreeY;
extern float leftWindowsWidth;
extern float rightWindowsWidth;

// Cached versions of lazy properties used for updates
namespace lazy {
extern TransparencyMode transparencyMode;
extern ProjectionMode projectionMode;
extern int transparencyRenderPasses;
extern int ssaaFactor;
extern float uiScale;
extern bool groundPlaneEnabled;
extern GroundPlaneMode groundPlaneMode;
extern ScaledValue<float> groundPlaneHeightFactor;
extern int shadowBlurIters;
extern float shadowDarkness;
} // namespace lazy


} // namespace internal
} // namespace polyscope

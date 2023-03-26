// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <string>


namespace polyscope {

// forward declaration
class FloatingQuantityStructure;


namespace internal {


// Various nitty-gritty internal details of Polyscope, which end users certainly should not touch or depend on.

uint64_t getNextUniqueID();

extern bool pointCloudEfficiencyWarningReported;

extern FloatingQuantityStructure* globalFloatingQuantityStructure;


} // namespace internal
} // namespace polyscope

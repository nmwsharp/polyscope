// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <cstdint>
#include <string>


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

} // namespace internal
} // namespace polyscope

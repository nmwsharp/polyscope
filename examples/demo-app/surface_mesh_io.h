// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/utilities.h"

namespace polyscope {

void loadPolygonSoup_OBJ(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                         std::vector<std::vector<size_t>>& faceIndicesOut);

void loadPolygonSoup_PLY(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                         std::vector<std::vector<size_t>>& faceIndicesOut);

// Load a mesh from a general file, detecting type from filename
void loadPolygonSoup(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                     std::vector<std::vector<size_t>>& faceIndicesOut);
} // namespace polyscope

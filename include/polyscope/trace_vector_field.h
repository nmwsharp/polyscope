#pragma once

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/vector3.h"

namespace polyscope {

// Trace lines through a vector field on a mesh.
// Return is a list of lines, each entry is (position, normal)
// Input field should be identified (raised to power), not disambiguated
// Settings 0 for nLines results in an automatically computed value
std::vector<std::vector<std::array<geometrycentral::Vector3, 2>>>
traceField(geometrycentral::Geometry<geometrycentral::Euclidean>* geometry,
           const geometrycentral::FaceData<std::complex<double>>& field, int nSym = 1, size_t nLines = 0);

} // namespace polyscope
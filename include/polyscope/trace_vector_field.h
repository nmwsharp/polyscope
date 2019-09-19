// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/surface_mesh.h"

namespace polyscope {

// Trace lines through a vector field on a mesh.
// Return is a list of lines, each entry is (position, normal)
// Input field should be identified (raised to power), not disambiguated
// Settings 0 for nLines results in an automatically computed value
std::vector<std::vector<std::array<glm::vec3, 2>>> traceField(SurfaceMesh& mesh, const std::vector<glm::vec2>& field,
                                                              int nSym = 1, size_t nLines = 0);

// Rotate in to a new basis in R3. Vector is rotated in to new tangent plane, then a change of basis is performed to
// the new basis. Basis vectors MUST be unit and orthogonal -- this function doesn't check.
glm::vec2 rotateToTangentBasis(glm::vec2 v, const glm::vec3& oldBasisX, const glm::vec3& oldBasisY,
                               const glm::vec3& newBasisX, const glm::vec3& newBasisY);


} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

// Trace lines through a vector field on a mesh.
// Return is a list of lines, each entry is (position, normal)
// Input field should be identified (raised to power), not disambiguated
// Settings 0 for nLines results in an automatically computed value
// std::vector<std::vector<std::array<glm::vec3, 2>>> traceField(HalfedgeMesh& mesh, const std::vector<Complex>& field,
// int nSym = 1, size_t nLines = 0);


} // namespace polyscope

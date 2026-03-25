// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh_quantity.h"

#include "polyscope/simple_triangle_mesh.h"

namespace polyscope {

SimpleTriangleMeshQuantity::SimpleTriangleMeshQuantity(std::string name, SimpleTriangleMesh& parentStructure,
                                                       bool dominates)
    : Quantity(name, parentStructure, dominates), parent(parentStructure) {}

} // namespace polyscope

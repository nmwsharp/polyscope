// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/utilities.h"

#include <stdexcept>

namespace polyscope {

// Shorthand to add a mesh to polyscope
template <class V, class F>
SimpleTriangleMesh* registerSimpleTriangleMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  checkInitialized();

  SimpleTriangleMesh* s = new SimpleTriangleMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions),
                                                 standardizeVectorArray<glm::uvec3, 3>(faceIndices));

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}


template <class V>
void SimpleTriangleMesh::updateVertices(const V& newPositions) {
  validateSize(newPositions, vertices.size(), "newPositions");
  vertices.data = standardizeVectorArray<glm::vec3, 3>(newPositions);
  vertices.markHostBufferUpdated();
}

template <class V, class F>
void SimpleTriangleMesh::update(const V& newPositions, const F& newFaces) {

  vertices.data = standardizeVectorArray<glm::vec3, 3>(newPositions);
  vertices.markHostBufferUpdated();

  faces.data = standardizeVectorArray<glm::uvec3, 3>(newFaces);
  faces.markHostBufferUpdated();
}

// Shorthand to get a mesh from polyscope
inline SimpleTriangleMesh* getSimpleTriangleMesh(std::string name) {
  return dynamic_cast<SimpleTriangleMesh*>(getStructure(SimpleTriangleMesh::structureTypeName, name));
}
inline bool hasSimpleTriangleMesh(std::string name) {
  return hasStructure(SimpleTriangleMesh::structureTypeName, name);
}
inline void removeSimpleTriangleMesh(std::string name, bool errorIfAbsent) {
  removeStructure(SimpleTriangleMesh::structureTypeName, name, errorIfAbsent);
}

} // namespace polyscope

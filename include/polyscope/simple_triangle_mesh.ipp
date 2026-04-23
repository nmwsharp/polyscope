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

// =====================================================
// ============== Quantities
// =====================================================

template <class T>
SimpleTriangleMeshVertexScalarQuantity* SimpleTriangleMesh::addVertexScalarQuantity(std::string name, const T& values,
                                                                                    DataType type) {
  validateSize(values, nVertices(), "vertex scalar quantity " + name);
  return addVertexScalarQuantityImpl(name, standardizeArray<float, T>(values), type);
}

template <class T>
SimpleTriangleMeshFaceScalarQuantity* SimpleTriangleMesh::addFaceScalarQuantity(std::string name, const T& values,
                                                                                DataType type) {
  validateSize(values, nFaces(), "face scalar quantity " + name);
  return addFaceScalarQuantityImpl(name, standardizeArray<float, T>(values), type);
}

template <class T>
SimpleTriangleMeshVertexColorQuantity* SimpleTriangleMesh::addVertexColorQuantity(std::string name, const T& values) {
  validateSize(values, nVertices(), "vertex color quantity " + name);
  return addVertexColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(values));
}

template <class T>
SimpleTriangleMeshFaceColorQuantity* SimpleTriangleMesh::addFaceColorQuantity(std::string name, const T& values) {
  validateSize(values, nFaces(), "face color quantity " + name);
  return addFaceColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(values));
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

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

namespace polyscope {

// Shorthand to add a mesh to polyscope

template <class V, class F>
VolumeMesh* registerTetMesh(std::string name, const V& vertexPositions, const F& tetIndices) {
  checkInitialized();

  // Standardize the array, and pad out extra indices with -1 for our representation
  std::vector<std::array<uint32_t, 8>> tetIndsArr = standardizeVectorArray<std::array<uint32_t, 8>, 4>(tetIndices);
  for (size_t iC = 0; iC < tetIndsArr.size(); iC++) {
    for (size_t j = 4; j < 8; j++) {
      tetIndsArr[iC][j] = INVALID_IND_32;
    }
  }

  VolumeMesh* s = new VolumeMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions), tetIndsArr);

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}


template <class V, class F>
VolumeMesh* registerHexMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  checkInitialized();

  VolumeMesh* s = new VolumeMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions),
                                 standardizeVectorArray<std::array<uint32_t, 8>, 8>(faceIndices));

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

template <class V, class F>
VolumeMesh* registerVolumeMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  checkInitialized();

  VolumeMesh* s = new VolumeMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions),
                                 standardizeVectorArray<std::array<uint32_t, 8>, 8>(faceIndices));

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

template <class V, class Ft, class Fh>
VolumeMesh* registerTetHexMesh(std::string name, const V& vertexPositions, const Ft& tetIndices, const Fh& hexIndices) {
  checkInitialized();

  // Standardize the array, and pad out extra indices with -1 for our representation
  std::vector<std::array<uint32_t, 8>> tetIndsArr = standardizeVectorArray<std::array<uint32_t, 8>, 4>(tetIndices);
  for (size_t iC = 0; iC < tetIndsArr.size(); iC++) {
    for (size_t j = 4; j < 8; j++) {
      tetIndsArr[iC][j] = INVALID_IND_32;
    }
  }
  std::vector<std::array<uint32_t, 8>> hexIndsArr = standardizeVectorArray<std::array<uint32_t, 8>, 8>(hexIndices);

  // combine the arrays
  tetIndsArr.insert(tetIndsArr.end(), hexIndsArr.begin(), hexIndsArr.end());

  VolumeMesh* s = new VolumeMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions), tetIndsArr);

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}


template <class V>
void VolumeMesh::updateVertexPositions(const V& newPositions) {
  validateSize(newPositions, nVertices(), "newPositions");
  vertexPositions.data = standardizeVectorArray<glm::vec3, 3>(newPositions);
  vertexPositions.markHostBufferUpdated();
  geometryChanged();
}


// Shorthand to get a mesh from polyscope
inline VolumeMesh* getVolumeMesh(std::string name) {
  return dynamic_cast<VolumeMesh*>(getStructure(VolumeMesh::structureTypeName, name));
}
inline bool hasVolumeMesh(std::string name) { return hasStructure(VolumeMesh::structureTypeName, name); }
inline void removeVolumeMesh(std::string name, bool errorIfAbsent) {
  removeStructure(VolumeMesh::structureTypeName, name, errorIfAbsent);
}


// Make mesh element type printable
inline std::string getMeshElementTypeName(VolumeMeshElement type) {
  switch (type) {
  case VolumeMeshElement::VERTEX:
    return "vertex";
  case VolumeMeshElement::EDGE:
    return "edge";
  case VolumeMeshElement::FACE:
    return "face";
  case VolumeMeshElement::CELL:
    return "cell";
  }
  exception("broken");
  return "";
}
inline std::ostream& operator<<(std::ostream& out, const VolumeMeshElement value) {
  return out << getMeshElementTypeName(value);
}


// === Quantity adders

// These are generally small wrappers which do some error checks, apply an array adaptor, and hand off to a
// private non-templated ___Impl version which does the actual adding work.


template <class T>
VolumeMeshVertexColorQuantity* VolumeMesh::addVertexColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, nVertices(), "vertex color quantity " + name);
  return addVertexColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


template <class T>
VolumeMeshCellColorQuantity* VolumeMesh::addCellColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, nCells(), "cell color quantity " + name);
  return addCellColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}

template <class T>
VolumeMeshVertexScalarQuantity* VolumeMesh::addVertexScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, nVertices(), "vertex scalar quantity " + name);
  return addVertexScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
VolumeMeshCellScalarQuantity* VolumeMesh::addCellScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, nCells(), "cell scalar quantity " + name);
  return addCellScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
VolumeMeshVertexVectorQuantity* VolumeMesh::addVertexVectorQuantity(std::string name, const T& vectors,
                                                                    VectorType vectorType) {
  validateSize(vectors, nVertices(), "vertex vector quantity " + name);
  return addVertexVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

template <class T>
VolumeMeshCellVectorQuantity* VolumeMesh::addCellVectorQuantity(std::string name, const T& vectors,
                                                                VectorType vectorType) {
  validateSize(vectors, nCells(), "cell vector quantity " + name);
  return addCellVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

} // namespace polyscope

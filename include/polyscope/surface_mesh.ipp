// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
namespace polyscope {

template <class T>
void SurfaceMesh::setVertexPermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, nVertices(), "vertex permutation for " + name);
  vertexPerm = standardizeArray<size_t, T>(perm);

  vertexDataSize = expectedSize;
  if (vertexDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : vertexPerm) {
      vertexDataSize = std::max(vertexDataSize, i);
    }
  }
}

template <class T>
void SurfaceMesh::setFacePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, nFaces(), "face permutation for " + name);
  facePerm = standardizeArray<size_t, T>(perm);

  faceDataSize = expectedSize;
  if (faceDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : facePerm) {
      faceDataSize = std::max(faceDataSize, i);
    }
  }
}

template <class T>
void SurfaceMesh::setEdgePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, nEdges(), "edge permutation for " + name);
  edgePerm = standardizeArray<size_t, T>(perm);

  edgeDataSize = expectedSize;
  if (edgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : edgePerm) {
      edgeDataSize = std::max(edgeDataSize, i);
    }
  }
}

template <class T>
void SurfaceMesh::setHalfedgePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, nHalfedges(), "halfedge permutation for " + name);
  halfedgePerm = standardizeArray<size_t, T>(perm);

  halfedgeDataSize = expectedSize;
  if (halfedgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : halfedgePerm) {
      halfedgeDataSize = std::max(halfedgeDataSize, i);
    }
  }
}

template <class T>
void SurfaceMesh::setCornerPermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, nCorners(), "corner permutation for " + name);
  cornerPerm = standardizeArray<size_t, T>(perm);

  cornerDataSize = expectedSize;
  if (cornerDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : cornerPerm) {
      cornerDataSize = std::max(cornerDataSize, i);
    }
  }
}

template <class T>
void SurfaceMesh::setAllPermutations(const std::array<std::pair<T, size_t>, 5>& perms) {
  if (perms[0].first.size() > 0) {
    setVertexPermutation(perms[0].first, perms[0].second);
  }
  if (perms[1].first.size() > 0) {
    setFacePermutation(perms[1].first, perms[1].second);
  }
  if (perms[2].first.size() > 0) {
    setEdgePermutation(perms[2].first, perms[2].second);
  }
  if (perms[3].first.size() > 0) {
    setHalfedgePermutation(perms[3].first, perms[3].second);
  }
  if (perms[4].first.size() > 0) {
    setCornerPermutation(perms[4].first, perms[4].second);
  }
}

// === Quantity adders

// These are generally small wrappers which do some error checks, apply an array adaptor, and hand off to a
// private non-templated ___Impl version which does the actual adding work.

template <class T>
SurfaceVertexColorQuantity* SurfaceMesh::addVertexColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, vertexDataSize, "vertex color quantity " + name);
  return addVertexColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


template <class T>
SurfaceFaceColorQuantity* SurfaceMesh::addFaceColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, faceDataSize, "face color quantity " + name);
  return addFaceColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


inline SurfaceVertexCountQuantity*
SurfaceMesh::addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  for (auto p : values) {
    if (p.first >= vertexDataSize) {
      error("passed index " + std::to_string(p.first) + " to addVertexCountQuantity [" + name +
            "] which is greater than the number of vertices");
    }
  }
  return addVertexCountQuantityImpl(name, values);
}


inline SurfaceVertexIsolatedScalarQuantity*
SurfaceMesh::addVertexIsolatedScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values) {
  for (auto p : values) {
    if (p.first >= vertexDataSize) {
      error("passed index " + std::to_string(p.first) + " to addVertexIsolatedScalarQuantity [" + name +
            "] which is greater than the number of vertices");
    }
  }
  return addVertexIsolatedScalarQuantityImpl(name, values);
}


inline SurfaceFaceCountQuantity* SurfaceMesh::addFaceCountQuantity(std::string name,
                                                                   const std::vector<std::pair<size_t, int>>& values) {
  for (auto p : values) {
    if (p.first >= faceDataSize) {
      error("passed index " + std::to_string(p.first) + " to addFaceCountQuantity [" + name +
            "] which is greater than the number of faces");
    }
  }
  return addFaceCountQuantityImpl(name, values);
}

template <class T>
SurfaceDistanceQuantity* SurfaceMesh::addVertexDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "distance quantity " + name);
  return addVertexDistanceQuantityImpl(name, standardizeArray<double>(distances));
}

template <class T>
SurfaceDistanceQuantity* SurfaceMesh::addVertexSignedDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "signed distance quantity " + name);
  return addVertexSignedDistanceQuantityImpl(name, standardizeArray<double>(distances));
}


template <class P, class E>
SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantity(std::string name, const P& nodes, const E& edges) {
  return addSurfaceGraphQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(nodes),
                                     standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
}


// Standard a parameterization, defined at corners
template <class T>
SurfaceCornerParameterizationQuantity* SurfaceMesh::addParameterizationQuantity(std::string name, const T& coords,
                                                                                ParamCoordsType type) {
  validateSize(coords, cornerDataSize, "parameterization quantity " + name);
  return addParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}

// Parameterization defined at vertices, rather than corners
template <class T>
SurfaceVertexParameterizationQuantity* SurfaceMesh::addVertexParameterizationQuantity(std::string name, const T& coords,
                                                                                      ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);
  return addVertexParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}

// "local" parameterization defined at vertices. has different presets: type is WORLD and style is LOCAL
template <class T>
SurfaceVertexParameterizationQuantity* SurfaceMesh::addLocalParameterizationQuantity(std::string name, const T& coords,
                                                                                     ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);
  return addLocalParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}


template <class T>
SurfaceVertexScalarQuantity* SurfaceMesh::addVertexScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, vertexDataSize, "vertex scalar quantity " + name);
  return addVertexScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
SurfaceFaceScalarQuantity* SurfaceMesh::addFaceScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, faceDataSize, "face scalar quantity " + name);
  return addFaceScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
SurfaceEdgeScalarQuantity* SurfaceMesh::addEdgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, edgeDataSize, "edge scalar quantity " + name);
  return addEdgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
SurfaceHalfedgeScalarQuantity* SurfaceMesh::addHalfedgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, halfedgeDataSize, "halfedge scalar quantity " + name);
  return addHalfedgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
SurfaceVertexVectorQuantity* SurfaceMesh::addVertexVectorQuantity(std::string name, const T& vectors,
                                                                  VectorType vectorType) {
  validateSize(vectors, vertexDataSize, "vertex vector quantity " + name);
  return addVertexVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

template <class T>
SurfaceFaceVectorQuantity* SurfaceMesh::addFaceVectorQuantity(std::string name, const T& vectors,
                                                              VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face vector quantity " + name);
  return addFaceVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

template <class T>
SurfaceFaceIntrinsicVectorQuantity* SurfaceMesh::addFaceIntrinsicVectorQuantity(std::string name, const T& vectors,
                                                                                int nSym, VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face intrinsic vector quantity " + name);
  return addFaceIntrinsicVectorQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(vectors), nSym, vectorType);
}


template <class T>
SurfaceVertexIntrinsicVectorQuantity* SurfaceMesh::addVertexIntrinsicVectorQuantity(std::string name, const T& vectors,
                                                                                    int nSym, VectorType vectorType) {
  validateSize(vectors, vertexDataSize, "vertex intrinsic vector quantity " + name);
  return addVertexIntrinsicVectorQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(vectors), nSym, vectorType);
}

// Orientations is `true` if the canonical orientation of the edge points from the lower-indexed vertex to the
// higher-indexed vertex, and `false` otherwise.
template <class T, class O>
SurfaceOneFormIntrinsicVectorQuantity* SurfaceMesh::addOneFormIntrinsicVectorQuantity(std::string name, const T& data,
                                                                                      const O& orientations) {
  validateSize(data, edgeDataSize, "one form intrinsic vector quantity " + name);
  return addOneFormIntrinsicVectorQuantityImpl(name, standardizeArray<double, T>(data),
                                               standardizeArray<char, O>(orientations));
}


} // namespace polyscope

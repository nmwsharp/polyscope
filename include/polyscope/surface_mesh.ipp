// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
namespace polyscope {

// Shorthand to add a mesh to polyscope
template <class V, class F>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  SurfaceMesh* s = new SurfaceMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions),
                                   standardizeNestedList<size_t, F>(faceIndices));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}
template <class V, class F>
SurfaceMesh* registerSurfaceMesh2D(std::string name, const V& vertexPositions, const F& faceIndices) {

  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(vertexPositions);
  for (auto& v : positions3D) {
    v.z = 0.;
  }

  SurfaceMesh* s = new SurfaceMesh(name, positions3D, standardizeNestedList<size_t, F>(faceIndices));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

// Shorthand to add a mesh to polyscope while also setting permutations
template <class V, class F, class P>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices,
                                 const std::array<std::pair<P, size_t>, 5>& perms) {
  SurfaceMesh* s = registerSurfaceMesh(name, vertexPositions, faceIndices);

  if (s) {
    s->setAllPermutations(perms);
  }

  return s;
}

template <class V>
void SurfaceMesh::updateVertexPositions(const V& newPositions) {
  vertices = standardizeVectorArray<glm::vec3, 3>(newPositions);

  // Rebuild any necessary quantities
  geometryChanged();
}

template <class V>
void SurfaceMesh::updateVertexPositions2D(const V& newPositions2D) {
  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(newPositions2D);
  for (glm::vec3& v : positions3D) {
    v.z = 0.;
  }

  // Call the main version
  updateVertexPositions(positions3D);
}

inline glm::vec3 SurfaceMesh::faceCenter(size_t iF) {
  glm::vec3 faceCenter = glm::vec3{0., 0., 0.};
  const auto& face = faces[iF];
  size_t D = face.size();
  for (size_t j = 0; j < D; j++) {
    faceCenter += vertices[face[j]];
  }
  faceCenter /= D;
  return faceCenter;
}

// Shorthand to get a mesh from polyscope
inline SurfaceMesh* getSurfaceMesh(std::string name) {
  return dynamic_cast<SurfaceMesh*>(getStructure(SurfaceMesh::structureTypeName, name));
}
inline bool hasSurfaceMesh(std::string name) { return hasStructure(SurfaceMesh::structureTypeName, name); }
inline void removeSurfaceMesh(std::string name, bool errorIfAbsent) {
  removeStructure(SurfaceMesh::structureTypeName, name, errorIfAbsent);
}


// Make mesh element type printable
inline std::string getMeshElementTypeName(MeshElement type) {
  switch (type) {
  case MeshElement::VERTEX:
    return "vertex";
  case MeshElement::FACE:
    return "face";
  case MeshElement::EDGE:
    return "edge";
  case MeshElement::HALFEDGE:
    return "halfedge";
  case MeshElement::CORNER:
    return "corner";
  }
  throw std::runtime_error("broken");
}
inline std::ostream& operator<<(std::ostream& out, const MeshElement value) {
  return out << getMeshElementTypeName(value);
}


template <class T>
void SurfaceMesh::setVertexPermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, vertexDataSize, "vertex permutation for " + name);
  vertexPerm = standardizeArray<size_t, T>(perm);

  vertexDataSize = expectedSize;
  if (vertexDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : vertexPerm) {
      vertexDataSize = std::max(vertexDataSize, i + 1);
    }
  }
}

template <class T>
void SurfaceMesh::setFacePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, faceDataSize, "face permutation for " + name);
  facePerm = standardizeArray<size_t, T>(perm);

  faceDataSize = expectedSize;
  if (faceDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : facePerm) {
      faceDataSize = std::max(faceDataSize, i + 1);
    }
  }
}

template <class T>
void SurfaceMesh::setEdgePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, edgeDataSize, "edge permutation for " + name);
  edgePerm = standardizeArray<size_t, T>(perm);

  edgeDataSize = expectedSize;
  if (edgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : edgePerm) {
      edgeDataSize = std::max(edgeDataSize, i + 1);
    }
  }
}

template <class T>
void SurfaceMesh::setHalfedgePermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, halfedgeDataSize, "halfedge permutation for " + name);
  halfedgePerm = standardizeArray<size_t, T>(perm);

  halfedgeDataSize = expectedSize;
  if (halfedgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : halfedgePerm) {
      halfedgeDataSize = std::max(halfedgeDataSize, i + 1);
    }
  }
}

template <class T>
void SurfaceMesh::setCornerPermutation(const T& perm, size_t expectedSize) {

  validateSize(perm, cornerDataSize, "corner permutation for " + name);
  cornerPerm = standardizeArray<size_t, T>(perm);

  cornerDataSize = expectedSize;
  if (cornerDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : cornerPerm) {
      cornerDataSize = std::max(cornerDataSize, i + 1);
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

template <class T>
SurfaceVertexScalarQuantity* SurfaceMesh::addVertexDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "distance quantity " + name);
  return addVertexDistanceQuantityImpl(name, standardizeArray<double>(distances));
}

template <class T>
SurfaceVertexScalarQuantity* SurfaceMesh::addVertexSignedDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "signed distance quantity " + name);
  return addVertexSignedDistanceQuantityImpl(name, standardizeArray<double>(distances));
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


template <class P, class E>
SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantity(std::string name, const P& nodes, const E& edges) {
  return addSurfaceGraphQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(nodes),
                                     standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
}

template <class P, class E>
SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantity2D(std::string name, const P& nodes, const E& edges) {

  std::vector<glm::vec3> nodes3D = standardizeVectorArray<glm::vec3, 2>(nodes);
  for (glm::vec3& v : nodes3D) {
    v.z = 0.;
  }

  return addSurfaceGraphQuantityImpl(name, nodes3D, standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
}


template <class P>
SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantity(std::string name, const std::vector<P>& paths) {

  // Convert to vector of paths
  std::vector<std::vector<glm::vec3>> convertPaths;
  for (const P& inputP : paths) {
    convertPaths.emplace_back(standardizeVectorArray<glm::vec3, 3>(inputP));
  }

  // Build flat list of nodes and edges between them
  std::vector<glm::vec3> nodes;
  std::vector<std::array<size_t, 2>> edges;
  for (auto& p : convertPaths) {
    for (size_t i = 0; i < p.size(); i++) {
      size_t N = nodes.size();
      nodes.push_back(p[i]);
      if (i > 0) {
        edges.push_back({N - 1, N});
      }
    }
  }

  return addSurfaceGraphQuantityImpl(name, nodes, edges);
}

template <class P>
SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantity2D(std::string name, const std::vector<P>& paths) {

  // Convert and call the general version
  std::vector<std::vector<glm::vec3>> paths3D;
  for (const P& inputP : paths) {
    std::vector<glm::vec3> p = standardizeVectorArray<glm::vec3, 2>(inputP);
    for (glm::vec3& v : p) {
      v.z = 0.;
    }
    paths3D.emplace_back(p);
  }

  return addSurfaceGraphQuantity(name, paths3D);
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
SurfaceVertexVectorQuantity* SurfaceMesh::addVertexVectorQuantity2D(std::string name, const T& vectors,
                                                                    VectorType vectorType) {
  validateSize(vectors, vertexDataSize, "vertex vector quantity " + name);
  std::vector<glm::vec3> vectors3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addVertexVectorQuantityImpl(name, vectors3D, vectorType);
}

template <class T>
SurfaceFaceVectorQuantity* SurfaceMesh::addFaceVectorQuantity(std::string name, const T& vectors,
                                                              VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face vector quantity " + name);
  return addFaceVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}
template <class T>
SurfaceFaceVectorQuantity* SurfaceMesh::addFaceVectorQuantity2D(std::string name, const T& vectors,
                                                                VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face vector quantity " + name);
  std::vector<glm::vec3> vectors3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addFaceVectorQuantityImpl(name, vectors3D, vectorType);
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


template <class T>
void SurfaceMesh::setVertexTangentBasisX(const T& vectors) {
  validateSize(vectors, vertexDataSize, "vertex tangent basis X");
  setVertexTangentBasisXImpl(standardizeVectorArray<glm::vec3, 3>(vectors));
}

template <class T>
void SurfaceMesh::setVertexTangentBasisX2D(const T& vectors) {
  validateSize(vectors, vertexDataSize, "vertex tangent basis X");

  std::vector<glm::vec3> vec3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (glm::vec3& v : vec3D) {
    v.z = 0.;
  }

  setVertexTangentBasisXImpl(vec3D);
}

template <class T>
void SurfaceMesh::setFaceTangentBasisX(const T& vectors) {
  validateSize(vectors, faceDataSize, "face tangent basis X");
  setFaceTangentBasisXImpl(standardizeVectorArray<glm::vec3, 3>(vectors));
}

template <class T>
void SurfaceMesh::setFaceTangentBasisX2D(const T& vectors) {
  validateSize(vectors, faceDataSize, "face tangent basis X");

  std::vector<glm::vec3> vec3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (glm::vec3& v : vec3D) {
    v.z = 0.;
  }

  setFaceTangentBasisXImpl(vec3D);
}


} // namespace polyscope

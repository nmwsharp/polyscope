// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
namespace polyscope {

// Shorthand to add a mesh to polyscope
template <class V, class F>
VolumeMesh* registerVolumeMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  VolumeMesh* s = new VolumeMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions),
                                 standardizeVectorArray<std::array<int64_t, 8>, 8>(faceIndices));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  // TODO

  return s;
}

template <class V>
void VolumeMesh::updateVertexPositions(const V& newPositions) {
  vertices = standardizeVectorArray<glm::vec3, 3>(newPositions);

  // Rebuild any necessary quantities
  refresh();
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
  throw std::runtime_error("broken");
}
inline std::ostream& operator<<(std::ostream& out, const VolumeMeshElement value) {
  return out << getMeshElementTypeName(value);
}


// === Quantity adders

// These are generally small wrappers which do some error checks, apply an array adaptor, and hand off to a
// private non-templated ___Impl version which does the actual adding work.


template <class T>
VolumeMeshVertexColorQuantity* VolumeMesh::addVertexColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, vertexDataSize, "vertex color quantity " + name);
  return addVertexColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


template <class T>
VolumeMeshCellColorQuantity* VolumeMesh::addCellColorQuantity(std::string name, const T& colors) {
  validateSize<T>(colors, cellDataSize, "cell color quantity " + name);
  return addCellColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}

/*

template <class T>
VolumeVertexScalarQuantity* VolumeMesh::addVertexDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "distance quantity " + name);
  return addVertexDistanceQuantityImpl(name, standardizeArray<double>(distances));
}

template <class T>
VolumeVertexScalarQuantity* VolumeMesh::addVertexSignedDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "signed distance quantity " + name);
  return addVertexSignedDistanceQuantityImpl(name, standardizeArray<double>(distances));
}

// Standard a parameterization, defined at corners
template <class T>
VolumeCornerParameterizationQuantity* VolumeMesh::addParameterizationQuantity(std::string name, const T& coords,
                                                                                ParamCoordsType type) {
  validateSize(coords, cornerDataSize, "parameterization quantity " + name);
  return addParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}

// Parameterization defined at vertices, rather than corners
template <class T>
VolumeVertexParameterizationQuantity* VolumeMesh::addVertexParameterizationQuantity(std::string name, const T& coords,
                                                                                      ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);
  return addVertexParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}

// "local" parameterization defined at vertices. has different presets: type is WORLD and style is LOCAL
template <class T>
VolumeVertexParameterizationQuantity* VolumeMesh::addLocalParameterizationQuantity(std::string name, const T& coords,
                                                                                     ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);
  return addLocalParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(coords), type);
}


inline VolumeVertexCountQuantity*
VolumeMesh::addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  for (auto p : values) {
    if (p.first >= vertexDataSize) {
      error("passed index " + std::to_string(p.first) + " to addVertexCountQuantity [" + name +
            "] which is greater than the number of vertices");
    }
  }
  return addVertexCountQuantityImpl(name, values);
}


inline VolumeVertexIsolatedScalarQuantity*
VolumeMesh::addVertexIsolatedScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values) {
  for (auto p : values) {
    if (p.first >= vertexDataSize) {
      error("passed index " + std::to_string(p.first) + " to addVertexIsolatedScalarQuantity [" + name +
            "] which is greater than the number of vertices");
    }
  }
  return addVertexIsolatedScalarQuantityImpl(name, values);
}


inline VolumeFaceCountQuantity* VolumeMesh::addFaceCountQuantity(std::string name,
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
VolumeGraphQuantity* VolumeMesh::addVolumeGraphQuantity(std::string name, const P& nodes, const E& edges) {
  return addVolumeGraphQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(nodes),
                                     standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
}

template <class P, class E>
VolumeGraphQuantity* VolumeMesh::addVolumeGraphQuantity2D(std::string name, const P& nodes, const E& edges) {

  std::vector<glm::vec3> nodes3D = standardizeVectorArray<glm::vec3, 2>(nodes);
  for (glm::vec3& v : nodes3D) {
    v.z = 0.;
  }

  return addVolumeGraphQuantityImpl(name, nodes3D, standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
}


template <class P>
VolumeGraphQuantity* VolumeMesh::addVolumeGraphQuantity(std::string name, const std::vector<P>& paths) {

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

  return addVolumeGraphQuantityImpl(name, nodes, edges);
}

template <class P>
VolumeGraphQuantity* VolumeMesh::addVolumeGraphQuantity2D(std::string name, const std::vector<P>& paths) {

  // Convert and call the general version
  std::vector<std::vector<glm::vec3>> paths3D;
  for (const P& inputP : paths) {
    std::vector<glm::vec3> p = standardizeVectorArray<glm::vec3, 2>(inputP);
    for (glm::vec3& v : p) {
      v.z = 0.;
    }
    paths3D.emplace_back(p);
  }

  return addVolumeGraphQuantity(name, paths3D);
}


template <class T>
VolumeVertexScalarQuantity* VolumeMesh::addVertexScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, vertexDataSize, "vertex scalar quantity " + name);
  return addVertexScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
VolumeFaceScalarQuantity* VolumeMesh::addFaceScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, faceDataSize, "face scalar quantity " + name);
  return addFaceScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
VolumeEdgeScalarQuantity* VolumeMesh::addEdgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, edgeDataSize, "edge scalar quantity " + name);
  return addEdgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
VolumeHalfedgeScalarQuantity* VolumeMesh::addHalfedgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, halfedgeDataSize, "halfedge scalar quantity " + name);
  return addHalfedgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
VolumeVertexVectorQuantity* VolumeMesh::addVertexVectorQuantity(std::string name, const T& vectors,
                                                                  VectorType vectorType) {
  validateSize(vectors, vertexDataSize, "vertex vector quantity " + name);
  return addVertexVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}
template <class T>
VolumeVertexVectorQuantity* VolumeMesh::addVertexVectorQuantity2D(std::string name, const T& vectors,
                                                                    VectorType vectorType) {
  validateSize(vectors, vertexDataSize, "vertex vector quantity " + name);
  std::vector<glm::vec3> vectors3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addVertexVectorQuantityImpl(name, vectors3D, vectorType);
}

template <class T>
VolumeFaceVectorQuantity* VolumeMesh::addFaceVectorQuantity(std::string name, const T& vectors,
                                                              VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face vector quantity " + name);
  return addFaceVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}
template <class T>
VolumeFaceVectorQuantity* VolumeMesh::addFaceVectorQuantity2D(std::string name, const T& vectors,
                                                                VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face vector quantity " + name);
  std::vector<glm::vec3> vectors3D = standardizeVectorArray<glm::vec3, 2>(vectors);
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addFaceVectorQuantityImpl(name, vectors3D, vectorType);
}

template <class T>
VolumeFaceIntrinsicVectorQuantity* VolumeMesh::addFaceIntrinsicVectorQuantity(std::string name, const T& vectors,
                                                                                int nSym, VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face intrinsic vector quantity " + name);
  return addFaceIntrinsicVectorQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(vectors), nSym, vectorType);
}

*/

} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/utilities.h"
#include <stdexcept>
namespace polyscope {

// Shorthand to add a mesh to polyscope
template <class V, class F>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices) {
  checkInitialized();

  std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> nestedListTup =
      standardizeNestedList<uint32_t, uint32_t, F>(faceIndices);

  std::vector<uint32_t>& faceIndsEntries = std::get<0>(nestedListTup);
  std::vector<uint32_t>& faceIndsStart = std::get<1>(nestedListTup);

  SurfaceMesh* s =
      new SurfaceMesh(name, standardizeVectorArray<glm::vec3, 3>(vertexPositions), faceIndsEntries, faceIndsStart);

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}
template <class V, class F>
SurfaceMesh* registerSurfaceMesh2D(std::string name, const V& vertexPositions, const F& faceIndices) {
  checkInitialized();

  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(vertexPositions);
  for (auto& v : positions3D) {
    v.z = 0.;
  }

  return registerSurfaceMesh(name, positions3D, faceIndices);
}

template <class V>
void SurfaceMesh::updateVertexPositions(const V& newPositions) {
  validateSize(newPositions, vertexDataSize, "newPositions");
  vertexPositions.data = standardizeVectorArray<glm::vec3, 3>(newPositions);
  vertexPositions.markHostBufferUpdated();
  recomputeGeometryIfPopulated();
}


template <class V>
void SurfaceMesh::updateVertexPositions2D(const V& newPositions2D) {
  validateSize(newPositions2D, vertexDataSize, "newPositions2D");
  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(newPositions2D);
  for (glm::vec3& v : positions3D) {
    v.z = 0.;
  }

  // Call the main version
  updateVertexPositions(positions3D);
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
  exception("broken");
  return "";
}
inline std::ostream& operator<<(std::ostream& out, const MeshElement value) {
  return out << getMeshElementTypeName(value);
}


template <class T>
void SurfaceMesh::setEdgePermutation(const T& perm, size_t expectedSize) {

  // try to catch cases where it is set twice
  if (triangleAllEdgeInds.size() > 0) {
    exception("Attempting to set an edge permutation for SurfaceMesh " + name +
              ", but one is already set. Must be set exactly once.");
    return;
  }

  validateSize(perm, nEdges(), "edge permutation for " + name);
  edgePerm = standardizeArray<size_t, T>(perm);

  edgeDataSize = expectedSize;
  if (edgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : edgePerm) {
      edgeDataSize = std::max(edgeDataSize, i + 1);
    }
  }

  // now that we have edge indexing, enable edge-related stuff
  markEdgesAsUsed();
}

template <class T>
void SurfaceMesh::setHalfedgePermutation(const T& perm, size_t expectedSize) {

  // attempt to catch cases where the user sets a permutation after already adding quantities which would use the
  // permutation (this is unsupported and will cause bad things)
  if (triangleAllHalfedgeInds.size() > 0) {
    exception(
        "SurfaceMesh " + name +
        ": a halfedge index permutation was set after quantities have already used the default permutation. This is "
        "not supported, the halfedge index must be specified before any halfedge-value data is added.");
    return;
  }

  validateSize(perm, nHalfedges(), "halfedge permutation for " + name);
  halfedgePerm = standardizeArray<size_t, T>(perm);

  halfedgeDataSize = expectedSize;
  if (halfedgeDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : halfedgePerm) {
      halfedgeDataSize = std::max(halfedgeDataSize, i + 1);
    }
  }

  markHalfedgesAsUsed();
}

template <class T>
void SurfaceMesh::setCornerPermutation(const T& perm, size_t expectedSize) {

  // attempt to catch cases where the user sets a permutation after already adding quantities which would use the
  // permutation (this is unsupported and will cause bad things)
  if (triangleAllCornerInds.size() > 0) {
    exception(
        "SurfaceMesh " + name +
        ": a corner index permutation was set after quantities have already used the default permutation. This is "
        "not supported, the corner index must be specified before any corner-value data is added.");
    return;
  }

  validateSize(perm, nCorners(), "corner permutation for " + name);
  cornerPerm = standardizeArray<size_t, T>(perm);

  cornerDataSize = expectedSize;
  if (cornerDataSize == 0) {
    // Find max element to set the data size
    for (size_t i : cornerPerm) {
      cornerDataSize = std::max(cornerDataSize, i + 1);
    }
  }

  markCornersAsUsed();
}


template <class T>
void SurfaceMesh::setAllPermutations(const std::array<std::pair<T, size_t>, 5>& perms) {
  // (kept for backward compatbility only)
  // forward to the 3-arg version, ignoring the unused ones
  setAllPermutations(std::array<std::pair<T, size_t>, 2>{perms[2], perms[3], perms[4]});
}

template <class T>
void SurfaceMesh::setAllPermutations(const std::array<std::pair<T, size_t>, 3>& perms) {
  if (perms[0].first.size() > 0) {
    setEdgePermutation(perms[0].first, perms[0].second);
  }
  if (perms[1].first.size() > 0) {
    setHalfedgePermutation(perms[1].first, perms[1].second);
  }
  if (perms[2].first.size() > 0) {
    setCornerPermutation(perms[2].first, perms[2].second);
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
  if (edgeDataSize == INVALID_IND) {
    exception("SurfaceMesh " + name +
              " attempted to set edge-valued data, but this requires an edge ordering. Call setEdgePermutation().");
  }
  validateSize(data, edgeDataSize, "edge scalar quantity " + name);
  return addEdgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
SurfaceHalfedgeScalarQuantity* SurfaceMesh::addHalfedgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, halfedgeDataSize, "halfedge scalar quantity " + name);
  return addHalfedgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
SurfaceCornerScalarQuantity* SurfaceMesh::addCornerScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, cornerDataSize, "corner scalar quantity " + name);
  return addCornerScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
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

template <class T, class BX, class BY>
SurfaceFaceTangentVectorQuantity* SurfaceMesh::addFaceTangentVectorQuantity(std::string name, const T& vectors,
                                                                            const BX& basisX, const BY& basisY,
                                                                            int nSym, VectorType vectorType) {
  validateSize(vectors, faceDataSize, "face tangent vector data " + name);
  validateSize(basisX, faceDataSize, "face tangent vector basisX " + name);
  validateSize(basisY, faceDataSize, "face tangent vector basisY " + name);

  return addFaceTangentVectorQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(vectors),
                                          standardizeVectorArray<glm::vec3, 3>(basisX),
                                          standardizeVectorArray<glm::vec3, 3>(basisY), nSym, vectorType);
}
template <class T, class BX, class BY>
SurfaceVertexTangentVectorQuantity* SurfaceMesh::addVertexTangentVectorQuantity(std::string name, const T& vectors,
                                                                                const BX& basisX, const BY& basisY,
                                                                                int nSym, VectorType vectorType) {

  validateSize(vectors, vertexDataSize, "vertex tangent vector data " + name);
  validateSize(basisX, vertexDataSize, "vertex tangent vector basisX " + name);
  validateSize(basisY, vertexDataSize, "vertex tangent vector basisY " + name);

  return addVertexTangentVectorQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(vectors),
                                            standardizeVectorArray<glm::vec3, 3>(basisX),
                                            standardizeVectorArray<glm::vec3, 3>(basisY), nSym, vectorType);
}


// Orientations is `true` if the canonical orientation of the edge points from the lower-indexed vertex to the
// higher-indexed vertex, and `false` otherwise.
template <class T, class O>
SurfaceOneFormTangentVectorQuantity* SurfaceMesh::addOneFormTangentVectorQuantity(std::string name, const T& data,
                                                                                  const O& orientations) {
  if (edgeDataSize == INVALID_IND) {
    exception("SurfaceMesh " + name +
              " attempted to set edge-valued data, but this requires an edge ordering. Call setEdgePermutation().");
  }
  validateSize(data, edgeDataSize, "one form tangent vector quantity " + name);
  return addOneFormTangentVectorQuantityImpl(name, standardizeArray<double, T>(data),
                                             standardizeArray<char, O>(orientations));
}


} // namespace polyscope

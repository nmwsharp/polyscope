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

} // namespace polyscope

namespace polyscope {

template <class T>
void SurfaceMesh::setVertexPermutation(const T& perm) {

  validateSize(perm, nVertices(), "vertex permutation for " + name);
  vertexPerm = standardizeArray<size_t, T>(perm);

  // Find max element to set the data size
  vertexDataSize = 0;
  for (size_t i : vertexPerm) {
    vertexDataSize = std::max(vertexDataSize, i);
  }
}

template <class T>
void SurfaceMesh::setFacePermutation(const T& perm) {

  validateSize(perm, nFaces(), "face permutation for " + name);
  facePerm = standardizeArray<size_t, T>(perm);

  // Find max element to set the data size
  faceDataSize = 0;
  for (size_t i : facePerm) {
    faceDataSize = std::max(faceDataSize, i);
  }
}

template <class T>
void SurfaceMesh::setEdgePermutation(const T& perm) {

  validateSize(perm, nEdges(), "edge permutation for " + name);
  edgePerm = standardizeArray<size_t, T>(perm);

  // Find max element to set the data size
  edgeDataSize = 0;
  for (size_t i : edgePerm) {
    edgeDataSize = std::max(edgeDataSize, i);
  }
}

template <class T>
void SurfaceMesh::setHalfedgePermutation(const T& perm) {

  validateSize(perm, nHalfedges(), "halfedge permutation for " + name);
  halfedgePerm = standardizeArray<size_t, T>(perm);

  // Find max element to set the data size
  halfedgeDataSize = 0;
  for (size_t i : halfedgePerm) {
    halfedgeDataSize = std::max(halfedgeDataSize, i);
  }
}

template <class T>
void SurfaceMesh::setCornerPermutation(const T& perm) {

  validateSize(perm, nCorners(), "corner permutation for " + name);
  cornerPerm = standardizeArray<size_t, T>(perm);

  // Find max element to set the data size
  cornerDataSize = 0;
  for (size_t i : cornerPerm) {
    cornerDataSize = std::max(cornerDataSize, i);
  }
}

template <class T>
void SurfaceMesh::setAllPermutations(const std::array<T, 5>& perms) {
  if(perms[0].size() > 0) {
    setVertexPermutation(perms[0]);
  }
  if(perms[1].size() > 0) {
    setFacePermutation(perms[1]);
  }
  if(perms[2].size() > 0) {
    setEdgePermutation(perms[2]);
  }
  if(perms[3].size() > 0) {
    setHalfedgePermutation(perms[3]);
  }
  if(perms[4].size() > 0) {
    setCornerPermutation(perms[4]);
  }
}

} // namespace polyscope

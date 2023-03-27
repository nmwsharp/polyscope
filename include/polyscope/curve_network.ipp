// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {


// Shorthand to add a curve network to polyscope
template <class P, class E>
CurveNetwork* registerCurveNetwork(std::string name, const P& nodes, const E& edges) {
  checkInitialized();

  CurveNetwork* s = new CurveNetwork(name, standardizeVectorArray<glm::vec3, 3>(nodes),
                                     standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}
template <class P, class E>
CurveNetwork* registerCurveNetwork2D(std::string name, const P& nodes, const E& edges) {
  checkInitialized();

  std::vector<glm::vec3> points3D(standardizeVectorArray<glm::vec3, 2>(nodes));
  for (auto& v : points3D) {
    v.z = 0.;
  }
  CurveNetwork* s = new CurveNetwork(name, points3D, standardizeVectorArray<std::array<size_t, 2>, 2>(edges));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}


// Shorthand to add curve from a line of points
template <class P>
CurveNetwork* registerCurveNetworkLine(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);
  for (size_t iE = 1; iE < N; iE++) {
    edges.push_back({iE - 1, iE});
  }

  CurveNetwork* s = new CurveNetwork(name, standardizeVectorArray<glm::vec3, 3>(nodes), edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}
template <class P>
CurveNetwork* registerCurveNetworkLine2D(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);
  for (size_t iE = 1; iE < N; iE++) {
    edges.push_back({iE - 1, iE});
  }
  std::vector<glm::vec3> points3D(standardizeVectorArray<glm::vec3, 2>(nodes));
  for (auto& v : points3D) {
    v.z = 0.;
  }

  CurveNetwork* s = new CurveNetwork(name, points3D, edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}


template <class P>
CurveNetwork* registerCurveNetworkSegments(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);

  if (N % 2 != 0) {
    exception("registerCurveNetworkSegments should have an even number of nodes");
  }

  for (size_t iE = 0; iE < N; iE += 2) {
    edges.push_back({iE, iE + 1});
  }

  CurveNetwork* s = new CurveNetwork(name, standardizeVectorArray<glm::vec3, 3>(nodes), edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}
template <class P>
CurveNetwork* registerCurveNetworkSegments2D(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);

  if (N % 2 != 0) {
    exception("registerCurveNetworkSegments2D should have an even number of nodes");
  }

  for (size_t iE = 0; iE < N; iE += 2) {
    edges.push_back({iE, iE + 1});
  }

  std::vector<glm::vec3> points3D(standardizeVectorArray<glm::vec3, 2>(nodes));
  for (auto& v : points3D) {
    v.z = 0.;
  }

  CurveNetwork* s = new CurveNetwork(name, points3D, edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

// Shorthand to add curve from a loop of points
template <class P>
CurveNetwork* registerCurveNetworkLoop(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);
  for (size_t iE = 0; iE < N; iE++) {
    edges.push_back({iE, (iE + 1) % N});
  }

  CurveNetwork* s = new CurveNetwork(name, standardizeVectorArray<glm::vec3, 3>(nodes), edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}
template <class P>
CurveNetwork* registerCurveNetworkLoop2D(std::string name, const P& nodes) {
  checkInitialized();

  std::vector<std::array<size_t, 2>> edges;
  size_t N = adaptorF_size(nodes);
  for (size_t iE = 0; iE < N; iE++) {
    edges.push_back({iE, (iE + 1) % N});
  }
  std::vector<glm::vec3> points3D(standardizeVectorArray<glm::vec3, 2>(nodes));
  for (auto& v : points3D) {
    v.z = 0.;
  }

  CurveNetwork* s = new CurveNetwork(name, points3D, edges);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}


template <class V>
void CurveNetwork::updateNodePositions(const V& newPositions) {
  validateSize(newPositions, nNodes(), "newPositions");
  nodePositions.data = standardizeVectorArray<glm::vec3, 3>(newPositions);
  nodePositions.markHostBufferUpdated();
  recomputeGeometryIfPopulated();
}


template <class V>
void CurveNetwork::updateNodePositions2D(const V& newPositions2D) {
  validateSize(newPositions2D, nNodes(), "newPositions2D");
  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(newPositions2D);
  for (glm::vec3& v : positions3D) {
    v.z = 0.;
  }

  // Call the main version
  updateNodePositions(positions3D);
}

// Shorthand to get a curve network from polyscope
inline CurveNetwork* getCurveNetwork(std::string name) {
  return dynamic_cast<CurveNetwork*>(getStructure(CurveNetwork::structureTypeName, name));
}
inline bool hasCurveNetwork(std::string name) { return hasStructure(CurveNetwork::structureTypeName, name); }
inline void removeCurveNetwork(std::string name, bool errorIfAbsent) {
  removeStructure(CurveNetwork::structureTypeName, name, errorIfAbsent);
}


// =====================================================
// ============== Quantities
// =====================================================

template <class T>
CurveNetworkNodeColorQuantity* CurveNetwork::addNodeColorQuantity(std::string name, const T& colors) {
  validateSize(colors, nNodes(), "curve network node color quantity " + name);
  return addNodeColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}

template <class T>
CurveNetworkEdgeColorQuantity* CurveNetwork::addEdgeColorQuantity(std::string name, const T& colors) {
  validateSize(colors, nEdges(), "curve network edge color quantity " + name);
  return addEdgeColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


template <class T>
CurveNetworkNodeScalarQuantity* CurveNetwork::addNodeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, nNodes(), "curve network node scalar quantity " + name);
  return addNodeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}

template <class T>
CurveNetworkEdgeScalarQuantity* CurveNetwork::addEdgeScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, nEdges(), "curve network edge scalar quantity " + name);
  return addEdgeScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
CurveNetworkNodeVectorQuantity* CurveNetwork::addNodeVectorQuantity(std::string name, const T& vectors,
                                                                    VectorType vectorType) {
  validateSize(vectors, nNodes(), "curve network node vector quantity " + name);
  return addNodeVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

template <class T>
CurveNetworkNodeVectorQuantity* CurveNetwork::addNodeVectorQuantity2D(std::string name, const T& vectors,
                                                                      VectorType vectorType) {
  validateSize(vectors, nNodes(), "curve network node vector quantity " + name);

  std::vector<glm::vec3> vectors3D(standardizeVectorArray<glm::vec3, 2>(vectors));
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addNodeVectorQuantityImpl(name, vectors3D, vectorType);
}


template <class T>
CurveNetworkEdgeVectorQuantity* CurveNetwork::addEdgeVectorQuantity(std::string name, const T& vectors,
                                                                    VectorType vectorType) {
  validateSize(vectors, nEdges(), "curve network edge vector quantity " + name);
  return addEdgeVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}

template <class T>
CurveNetworkEdgeVectorQuantity* CurveNetwork::addEdgeVectorQuantity2D(std::string name, const T& vectors,
                                                                      VectorType vectorType) {
  validateSize(vectors, nEdges(), "curve network edge vector quantity " + name);

  std::vector<glm::vec3> vectors3D(standardizeVectorArray<glm::vec3, 2>(vectors));
  for (auto& v : vectors3D) {
    v.z = 0.;
  }
  return addEdgeVectorQuantityImpl(name, vectors3D, vectorType);
}


} // namespace polyscope

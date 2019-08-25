// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/curve_network_quantity.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/curve_network_color_quantity.h"
#include "polyscope/curve_network_scalar_quantity.h"
#include "polyscope/curve_network_vector_quantity.h"

#include <vector>

namespace polyscope {

// Forward declare curve network
class CurveNetwork;

// Forward declare quantity types
class CurveNetworkNodeScalarQuantity;
class CurveNetworkEdgeScalarQuantity;
class CurveNetworkNodeColorQuantity;
class CurveNetworkEdgeColorQuantity;
class CurveNetworkNodeVectorQuantity;
class CurveNetworkEdgeVectorQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<CurveNetwork> {
  typedef CurveNetworkQuantity type;
};

class CurveNetwork : public QuantityStructure<CurveNetwork> {
public:
  // === Member functions ===

  // Construct a new curve network structure
  CurveNetwork(std::string name, std::vector<glm::vec3> nodes, std::vector<std::array<size_t, 2>> edges);

  // === Overloads

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Render the the structure on screen
  virtual void draw() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;

  virtual std::string typeName() override;

  // === Quantities

  // Scalars
  template <class T>
  CurveNetworkNodeScalarQuantity* addNodeScalarQuantity(std::string name, const T& values,
                                                        DataType type = DataType::STANDARD);
  template <class T>
  CurveNetworkEdgeScalarQuantity* addEdgeScalarQuantity(std::string name, const T& values,
                                                        DataType type = DataType::STANDARD);

  // Colors
  template <class T>
  CurveNetworkNodeColorQuantity* addNodeColorQuantity(std::string name, const T& values);
  template <class T>
  CurveNetworkEdgeColorQuantity* addEdgeColorQuantity(std::string name, const T& values);

  // Vectors
  template <class T>
  CurveNetworkNodeVectorQuantity* addNodeVectorQuantity(std::string name, const T& vectors,
                                                        VectorType vectorType = VectorType::STANDARD);
  template <class T>
  CurveNetworkEdgeVectorQuantity* addEdgeVectorQuantity(std::string name, const T& vectors,
                                                        VectorType vectorType = VectorType::STANDARD);
  template <class T>
  CurveNetworkNodeVectorQuantity* addNodeVectorQuantity2D(std::string name, const T& vectors,
                                                          VectorType vectorType = VectorType::STANDARD);
  template <class T>
  CurveNetworkEdgeVectorQuantity* addEdgeVectorQuantity2D(std::string name, const T& vectors,
                                                          VectorType vectorType = VectorType::STANDARD);


  // === Members and utilities

  // The nodes that make up this curve network
  std::vector<glm::vec3> nodes;
  std::vector<size_t> nodeDegrees; // populated on construction
  size_t nNodes() const { return nodes.size(); }

  std::vector<std::array<size_t, 2>> edges;
  size_t nEdges() const { return edges.size(); }


  // Misc data
  static const std::string structureTypeName;

  // Small utilities
  void setCurveNetworkNodeUniforms(gl::GLProgram& p);
  void setCurveNetworkEdgeUniforms(gl::GLProgram& p);
  void fillEdgeGeometryBuffers(gl::GLProgram& program);
  void fillNodeGeometryBuffers(gl::GLProgram& program);

  // Visualization parameters
  glm::vec3 baseColor;
  float radius = 0.001;

  // === Mutate
  template <class V>
  void updateNodePositions(const V& newPositions);
  template <class V>
  void updateNodePositions2D(const V& newPositions);

private:
  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::unique_ptr<gl::GLProgram> edgeProgram;
  std::unique_ptr<gl::GLProgram> nodeProgram;
  std::unique_ptr<gl::GLProgram> edgePickProgram;
  std::unique_ptr<gl::GLProgram> nodePickProgram;

  // === Helpers

  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();

  // Pick helpers
  void buildNodePickUI(size_t nodeInd);
  void buildEdgePickUI(size_t edgeInd);

  // === Quantity adder implementations
  // clang-format off
  CurveNetworkNodeScalarQuantity* addNodeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  CurveNetworkEdgeScalarQuantity* addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  CurveNetworkNodeColorQuantity* addNodeColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  CurveNetworkEdgeColorQuantity* addEdgeColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  CurveNetworkNodeVectorQuantity* addNodeVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  CurveNetworkEdgeVectorQuantity* addEdgeVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  // clang-format on
};


// Shorthand to add a curve network to polyscope
template <class P, class E>
CurveNetwork* registerCurveNetwork(std::string name, const P& points, const E& edges);
template <class P, class E>
CurveNetwork* registerCurveNetwork2D(std::string name, const P& points, const E& edges);


// Shorthand to add a curve network, automatically constructing the connectivity of a line
template <class P>
CurveNetwork* registerCurveNetworkLine(std::string name, const P& points);
template <class P>
CurveNetwork* registerCurveNetworkLine2D(std::string name, const P& points);


// Shorthand to add a curve network, automatically constructing the connectivity of a loop
template <class P>
CurveNetwork* registerCurveNetworkLoop(std::string name, const P& points);
template <class P>
CurveNetwork* registerCurveNetworkLoop2D(std::string name, const P& points);

// Shorthand to get a curve network from polyscope
inline CurveNetwork* getCurveNetwork(std::string name = "");


} // namespace polyscope

#include "polyscope/curve_network.ipp"

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/curve_network.h"
#include "polyscope/ribbon_artist.h"

namespace polyscope {

// ==== Common base class

// Represents a vector field associated with a curve network
class CurveNetworkVectorQuantity : public CurveNetworkQuantity {
public:
  CurveNetworkVectorQuantity(std::string name, CurveNetwork& network_, VectorType vectorType_ = VectorType::STANDARD);


  virtual void draw() override;
  virtual void buildCustomUI() override;

  // Allow children to append to the UI
  virtual void drawSubUI();

  // === Members
  const VectorType vectorType;
  std::vector<glm::vec3> vectorRoots;
  std::vector<glm::vec3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  glm::vec3 vectorColor;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<glm::vec3> mapper;

  void writeToFile(std::string filename = "");

  // GL things
  void prepareProgram();
  std::unique_ptr<gl::GLProgram> program;

protected:
  // Set up the mapper for vectors
  void prepareVectorMapper();
};


// ==== R3 vectors at nodes

class CurveNetworkNodeVectorQuantity : public CurveNetworkVectorQuantity {
public:
  CurveNetworkNodeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, CurveNetwork& network_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual std::string niceName() override;
  virtual void buildNodeInfoGUI(size_t vInd) override;
  virtual void geometryChanged() override;
};


// ==== R3 vectors at edges

class CurveNetworkEdgeVectorQuantity : public CurveNetworkVectorQuantity {
public:
  CurveNetworkEdgeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, CurveNetwork& network_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual std::string niceName() override;
  virtual void buildEdgeInfoGUI(size_t fInd) override;
  virtual void geometryChanged() override;
};


} // namespace polyscope

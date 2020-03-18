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

  // === Option accessors

  //  The vectors will be scaled such that the longest vector is this long
  CurveNetworkVectorQuantity* setVectorLengthScale(double newLength, bool isRelative = true);
  double getVectorLengthScale();

  // The radius of the vectors
  CurveNetworkVectorQuantity* setVectorRadius(double val, bool isRelative = true);
  double getVectorRadius();

  // The color of the vectors
  CurveNetworkVectorQuantity* setVectorColor(glm::vec3 color);
  glm::vec3 getVectorColor();

  // Material
  CurveNetworkVectorQuantity* setMaterial(std::string name);
  std::string getMaterial();

  void writeToFile(std::string filename = "");

protected:
  // === Visualization options
  PersistentValue<ScaledValue<float>> vectorLengthMult;
  PersistentValue<ScaledValue<float>> vectorRadius;
  PersistentValue<glm::vec3> vectorColor;
  PersistentValue<std::string> material;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<glm::vec3> mapper;

  // GL things
  void prepareProgram();
  std::shared_ptr<render::ShaderProgram> program;

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

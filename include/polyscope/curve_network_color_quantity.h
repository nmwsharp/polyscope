// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/curve_network.h"

namespace polyscope {

// forward declaration
class CurveNetworkMeshQuantity;
class CurveNetwork;

class CurveNetworkColorQuantity : public CurveNetworkQuantity {
public:
  CurveNetworkColorQuantity(std::string name, CurveNetwork& network_, std::string definedOn);

  virtual void draw() override;
  virtual std::string niceName() override;

  virtual void refresh() override;

protected:
  // UI internals
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> nodeProgram;
  std::shared_ptr<render::ShaderProgram> edgeProgram;

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Node Color             ==========
// ========================================================

class CurveNetworkNodeColorQuantity : public CurveNetworkColorQuantity {
public:
  CurveNetworkNodeColorQuantity(std::string name, std::vector<glm::vec3> values_, CurveNetwork& network_);

  virtual void createProgram() override;

  void buildNodeInfoGUI(size_t vInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

// ========================================================
// ==========             Edge Color             ==========
// ========================================================

class CurveNetworkEdgeColorQuantity : public CurveNetworkColorQuantity {
public:
  CurveNetworkEdgeColorQuantity(std::string name, std::vector<glm::vec3> values_, CurveNetwork& network_);

  virtual void createProgram() override;

  void buildEdgeInfoGUI(size_t eInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

} // namespace polyscope

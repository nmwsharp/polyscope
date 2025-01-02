// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/curve_network.h"
#include "polyscope/vector_quantity.h"

namespace polyscope {

// ==== Common base class

// Represents a vector field associated with a curve network
// NOTE: This intermediate class is not really necessary anymore; it is subsumed by the VectorQuantity<> classes which
// serve as common bases for ALL vector types. At this point it is just kept around for backward compatibility, to not
// break user code which holds a reference to it.
class CurveNetworkVectorQuantity : public CurveNetworkQuantity {
public:
  CurveNetworkVectorQuantity(std::string name, CurveNetwork& network_);

  // === Option accessors

protected:
};


// ==== R3 vectors at nodes

class CurveNetworkNodeVectorQuantity : public CurveNetworkVectorQuantity,
                                       public VectorQuantity<CurveNetworkNodeVectorQuantity> {
public:
  CurveNetworkNodeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, CurveNetwork& network_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void refresh() override;
  virtual void buildNodeInfoGUI(size_t vInd) override;
};


// ==== R3 vectors at edges

class CurveNetworkEdgeVectorQuantity : public CurveNetworkVectorQuantity,
                                       public VectorQuantity<CurveNetworkEdgeVectorQuantity> {
public:
  CurveNetworkEdgeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, CurveNetwork& network_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void refresh() override;
  virtual void buildEdgeInfoGUI(size_t vInd) override;
};


} // namespace polyscope

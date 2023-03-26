// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/types.h"
#include "polyscope/vector_quantity.h"
#include "polyscope/volume_mesh.h"

namespace polyscope {

// ==== Common base class

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and R2 fields embedded in the surface

// NOTE: This intermediate class is not really necessary anymore; it is subsumed by the VectorQuantity<> classes which
// serve as common bases for ALL vector types. At this point it is just kept around for backward compatibility, to not
// break user code which holds a reference to it.
class VolumeMeshVectorQuantity : public VolumeMeshQuantity {
public:
  VolumeMeshVectorQuantity(std::string name, VolumeMesh& mesh_, VolumeMeshElement definedOn_);

protected:
  VolumeMeshElement definedOn;
};


// ==== R3 vectors at vertices

class VolumeMeshVertexVectorQuantity : public VolumeMeshVectorQuantity,
                                       public VectorQuantity<VolumeMeshVertexVectorQuantity> {
public:
  VolumeMeshVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, VolumeMesh& mesh_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildVertexInfoGUI(size_t vInd) override;
};


// ==== R3 vectors at cells

class VolumeMeshCellVectorQuantity : public VolumeMeshVectorQuantity,
                                     public VectorQuantity<VolumeMeshCellVectorQuantity> {
public:
  VolumeMeshCellVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, VolumeMesh& mesh_,
                               VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildCellInfoGUI(size_t cInd) override;
};


} // namespace polyscope

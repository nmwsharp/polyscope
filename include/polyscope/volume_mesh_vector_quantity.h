// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/types.h"
#include "polyscope/vector_artist.h"
#include "polyscope/volume_mesh.h"

namespace polyscope {

// ==== Common base class

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and R2 fields embedded in the surface
class VolumeMeshVectorQuantity : public VolumeMeshQuantity {
public:
  VolumeMeshVectorQuantity(std::string name, VolumeMesh& mesh_, VolumeMeshElement definedOn_,
                           VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  // Allow children to append to the UI
  virtual void drawSubUI();

  // === Members

  // Note: these vectors are not the raw vectors passed in by the user, but have been rescaled such that the longest has
  // length 1 (unless type is VectorType::Ambient)
  const VectorType vectorType;

  // The actual data
  std::vector<glm::vec3> vectors;
  std::vector<glm::vec3> vectorRoots;

  // === Option accessors

  //  The vectors will be scaled such that the longest vector is this long
  VolumeMeshVectorQuantity* setVectorLengthScale(double newLength, bool isRelative = true);
  double getVectorLengthScale();

  // The radius of the vectors
  VolumeMeshVectorQuantity* setVectorRadius(double val, bool isRelative = true);
  double getVectorRadius();

  // The color of the vectors
  VolumeMeshVectorQuantity* setVectorColor(glm::vec3 color);
  glm::vec3 getVectorColor();

  // Material
  VolumeMeshVectorQuantity* setMaterial(std::string name);
  std::string getMaterial();

protected:
  // Manages _actually_ drawing the vectors, generating gui.
  std::unique_ptr<VectorArtist> vectorArtist;
  void prepareVectorArtist();

  VolumeMeshElement definedOn;
};


// ==== R3 vectors at vertices

class VolumeMeshVertexVectorQuantity : public VolumeMeshVectorQuantity {
public:
  VolumeMeshVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, VolumeMesh& mesh_,
                                 VectorType vectorType_ = VectorType::STANDARD);

  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildVertexInfoGUI(size_t vInd) override;
};


// ==== R3 vectors at cells

class VolumeMeshCellVectorQuantity : public VolumeMeshVectorQuantity {
public:
  VolumeMeshCellVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, VolumeMesh& mesh_,
                               VectorType vectorType_ = VectorType::STANDARD);

  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildCellInfoGUI(size_t cInd) override;
};


} // namespace polyscope

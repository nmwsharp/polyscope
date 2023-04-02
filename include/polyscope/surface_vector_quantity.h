// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/vector_quantity.h"

namespace polyscope {

// ==== Common base class

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and R2 fields embedded in the surface

// NOTE: This intermediate class is not really necessary anymore; it is subsumed by the VectorQuantity<> classes which
// serve as common bases for ALL vector types. At this point it is just kept around for backward compatibility, to not
// break user code which holds a reference to it.
class SurfaceVectorQuantity : public SurfaceMeshQuantity {
public:
  SurfaceVectorQuantity(std::string name, SurfaceMesh& mesh_, MeshElement definedOn_);

  // === Members

  // === Option accessors

protected:
  MeshElement definedOn;
};


// ==== R3 vectors at vertices

class SurfaceVertexVectorQuantity : public SurfaceVectorQuantity, public VectorQuantity<SurfaceVertexVectorQuantity> {
public:
  SurfaceVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, SurfaceMesh& mesh_,
                              VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildVertexInfoGUI(size_t vInd) override;
};


// ==== R3 vectors at faces

class SurfaceFaceVectorQuantity : public SurfaceVectorQuantity, public VectorQuantity<SurfaceFaceVectorQuantity> {
public:
  SurfaceFaceVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, SurfaceMesh& mesh_,
                            VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;
  virtual void buildFaceInfoGUI(size_t fInd) override;
};

// ==== Tangent vectors at faces

class SurfaceFaceTangentVectorQuantity : public SurfaceVectorQuantity,
                                         public TangentVectorQuantity<SurfaceFaceTangentVectorQuantity> {
public:
  SurfaceFaceTangentVectorQuantity(std::string name, std::vector<glm::vec2> vectors_, std::vector<glm::vec3> basisX_,
                                   std::vector<glm::vec3> basisY_, SurfaceMesh& mesh_, int nSym = 1,
                                   VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;
  void buildFaceInfoGUI(size_t fInd) override;
};


// ==== Tangent vectors at vertices

class SurfaceVertexTangentVectorQuantity : public SurfaceVectorQuantity,
                                           public TangentVectorQuantity<SurfaceVertexTangentVectorQuantity> {
public:
  SurfaceVertexTangentVectorQuantity(std::string name, std::vector<glm::vec2> vectors_, std::vector<glm::vec3> basisX_,
                                     std::vector<glm::vec3> basisY_, SurfaceMesh& mesh_, int nSym = 1,
                                     VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual std::string niceName() override;
  void buildVertexInfoGUI(size_t vInd) override;
};


// ==== Tangent one form on edges

class SurfaceOneFormTangentVectorQuantity : public SurfaceVectorQuantity,
                                            public TangentVectorQuantity<SurfaceOneFormTangentVectorQuantity> {
public:
  SurfaceOneFormTangentVectorQuantity(std::string name, std::vector<double> oneForm_, std::vector<char> orientations_,
                                      SurfaceMesh& mesh_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;

  std::vector<double> oneForm;
  std::vector<char> canonicalOrientation;

  void buildEdgeInfoGUI(size_t eInd) override;
};

} // namespace polyscope

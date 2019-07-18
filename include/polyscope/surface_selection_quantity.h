// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceSelectionQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceSelectionQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn);

  virtual void draw() override;
  virtual void drawUI() override;
  virtual void userEdit() = 0;

  // === Members
  bool allowEditingFromDefaultUI = true;

protected:
  // UI internals
  int iColorMap = 0;
  const std::string definedOn;
};

// ========================================================
// ==========           Vertex Selection         ==========
// ========================================================

class SurfaceSelectionVertexQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionVertexQuantity(std::string name, SurfaceMesh* mesh_);
  SurfaceSelectionVertexQuantity(std::string name, std::vector<char>& initialMembership_, SurfaceMesh* mesh_);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildVertexInfoGUI(size_t vInd) override;

  void userEdit() override;
  virtual void setProgramValues(gl::GLProgram* program) override;

  // === Members
  std::vector<char> membership; // 1 if in, 0 otherwise

private:
  // User membership editing
  void userEditCallback();
  bool membershipStale = false;
  int mouseMemberAction = 0;
};

template <class T>
void SurfaceMesh::addVertexSelectionQuantity(std::string name, const T& initialMembership) {
  std::shared_ptr<SurfaceSelectionVertexQuantity> q = std::make_shared<SurfaceSelectionVertexQuantity>(
      name, standardizeArray<char, T>(initialMembership, nVertices(), "vertex selection quantity " + name), this);
  addSurfaceQuantity(q);
}


/*
// ========================================================
// ==========            Face Selection          ==========
// ========================================================

class SurfaceSelectionFaceQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionFaceQuantity(std::string name, FaceData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(FacePtr f) override;

  // === Members
  FaceData<double> values;
};

// ========================================================
// ==========            Edge Selection          ==========
// ========================================================

class SurfaceSelectionEdgeQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionEdgeQuantity(std::string name, EdgeData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(EdgePtr e) override;


  // === Members
  EdgeData<double> values;
};

// ========================================================
// ==========          Halfedge Selection        ==========
// ========================================================

class SurfaceSelectionHalfedgeQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionHalfedgeQuantity(std::string name, HalfedgeData<double>& values_, SurfaceMesh* mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(HalfedgePtr he) override;

  // === Members
  HalfedgeData<double> values;
};
*/
} // namespace polyscope

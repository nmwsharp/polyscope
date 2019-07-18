// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceInputCurveQuantity : public SurfaceQuantity {
public:
  SurfaceInputCurveQuantity(std::string name, SurfaceMesh* mesh_);
  ~SurfaceInputCurveQuantity();

  virtual void draw() override;
  virtual void drawUI() override;
  void userEdit();

  void fillBuffers();

  void writeToFile(std::string filename = "");

  geometrycentral::MeshEmbeddedCurve getCurve();
  void setCurve(geometrycentral::MeshEmbeddedCurve& newCurve);

  // === Members
  bool allowEditingFromDefaultUI = true;

protected:
  geometrycentral::MeshEmbeddedCurve curve;

  // GL program
  gl::GLProgram* program = nullptr;
  bool bufferStale = true;

  // UI internals
  float radiusParam = 0.001;
  Color3f curveColor;
  void userEditCallback();
};


} // namespace polyscope

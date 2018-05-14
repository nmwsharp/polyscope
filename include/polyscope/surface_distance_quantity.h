#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceDistanceQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceDistanceQuantity(std::string name, VertexData<double>& values_, SurfaceMesh* mesh_, bool signedDist = false);

  void draw() override;
  void drawUI() override;
  void setProgramValues(gl::GLProgram* program) override;
  gl::GLProgram* createProgram() override;

  void buildInfoGUI(VertexPtr v) override;
  void fillColorBuffers(gl::GLProgram* p);
  
  void writeToFile(std::string filename = "");


  // === Members
  VertexData<double> distances;
  bool signedDist;

protected:
  // Affine data maps and limits
  void resetVizRange();
  float vizRangeLow, vizRangeHigh;
  float dataRangeHigh, dataRangeLow;
  Histogram hist;
  float modLen = 0.02;

  // UI internals
  int iColorMap = 0;
};

} // namespace polyscope


#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceDistanceQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceDistanceQuantity(std::string name, std::vector<double> values_, SurfaceMesh* mesh_, bool signedDist = false);

  void draw() override;
  void drawUI() override;
  void setProgramValues(gl::GLProgram* program) override;
  gl::GLProgram* createProgram() override;

  void buildVertexInfoGUI(size_t v) override;
  void fillColorBuffers(gl::GLProgram* p);
  
  void writeToFile(std::string filename = "");


  // === Members
  std::vector<double> distances;
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

template <class T>
void SurfaceMesh::addDistanceQuantity(std::string name, const T& distances) {
  std::shared_ptr<SurfaceDistanceQuantity> q = std::make_shared<SurfaceDistanceQuantity>(
      name, standardizeArray<double, T>(distances, nVertices, "distance quantity " + name), this, false);
  addSurfaceQuantity(q);
}

template <class T>
void SurfaceMesh::addSignedDistanceQuantity(std::string name, const T& distances) {
  std::shared_ptr<SurfaceDistanceQuantity> q = std::make_shared<SurfaceDistanceQuantity>(
      name, standardizeArray<double, T>(distances, nVertices, "signed distance quantity " + name), this, true);
  addSurfaceQuantity(q);
}


} // namespace polyscope


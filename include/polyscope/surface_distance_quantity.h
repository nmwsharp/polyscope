#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceDistanceQuantity : public SurfaceMeshQuantity {
public:
  SurfaceDistanceQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_, bool signedDist = false);

  void draw() override;
  virtual void buildCustomUI() override;

  virtual std::string niceName() override;
  virtual void geometryChanged() override;

  void buildVertexInfoGUI(size_t v) override;

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
  std::unique_ptr<gl::GLProgram> program;

  // Helpers
  void createProgram();
  void setProgramUniforms(gl::GLProgram& program);
  void fillColorBuffers(gl::GLProgram& p);
};

template <class T>
void SurfaceMesh::addVertexDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "distance quantity " + name);

  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(
      name, applyPermutation(standardizeArray<double, T>(distances), vertexPerm), *this, false);
  addQuantity(q);
}

template <class T>
void SurfaceMesh::addVertexSignedDistanceQuantity(std::string name, const T& distances) {
  validateSize(distances, vertexDataSize, "signed distance quantity " + name);

  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(
      name, applyPermutation(standardizeArray<double, T>(distances), vertexPerm), *this, true);
  addQuantity(q);
}


} // namespace polyscope


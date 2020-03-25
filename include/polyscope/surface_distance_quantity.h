// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
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

  // === Members
  std::vector<double> distances;
  const bool signedDist;
  
  // === Get/set visualization parameters

  // The color map
  SurfaceDistanceQuantity* setColorMap(std::string val);
  std::string getColorMap();

  // Length of isoline stripes
  SurfaceDistanceQuantity* setStripeSize(double stripeSize, bool isRelative=true);
  double getStripeSize();

  // Data limits mapped in to colormap
  SurfaceDistanceQuantity* setMapRange(std::pair<double, double> val);
  std::pair<double, double> getMapRange();
  SurfaceDistanceQuantity* resetMapRange(); // reset to full range

protected:

  // === Visualization parameters

  // Affine data maps and limits
  std::pair<float, float> vizRange;
  std::pair<double, double> dataRange;
  PersistentValue<ScaledValue<float>> stripeSize;
  Histogram hist;

  // UI internals
  PersistentValue<std::string> cMap;
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  void createProgram();
  void setProgramUniforms(render::ShaderProgram& program);
  void fillColorBuffers(render::ShaderProgram& p);
};

} // namespace polyscope

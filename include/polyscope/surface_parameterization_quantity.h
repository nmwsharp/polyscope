// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/parameterization_quantity.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"


namespace polyscope {


// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================


class SurfaceParameterizationQuantity : public SurfaceMeshQuantity,
                                        public ParameterizationQuantity<SurfaceParameterizationQuantity> {

public:
  SurfaceParameterizationQuantity(std::string name, SurfaceMesh& mesh_, const std::vector<glm::vec2>& coords_,
                                  ParamCoordsType type_, ParamVizStyle style_);

  virtual void draw() override;
  virtual void refresh() override;
  virtual void buildCustomUI() override;


protected:
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  void createProgram();
  virtual void fillCoordBuffers(render::ShaderProgram& p) = 0;
};


// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================

class SurfaceCornerParameterizationQuantity : public SurfaceParameterizationQuantity {

public:
  SurfaceCornerParameterizationQuantity(std::string name, SurfaceMesh& mesh_, const std::vector<glm::vec2>& coords_,
                                        ParamCoordsType type_, ParamVizStyle style);

  virtual void buildCornerInfoGUI(size_t cInd) override;
  virtual std::string niceName() override;

protected:
  virtual void fillCoordBuffers(render::ShaderProgram& p) override;
};


// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================

class SurfaceVertexParameterizationQuantity : public SurfaceParameterizationQuantity {
public:
  SurfaceVertexParameterizationQuantity(std::string name, SurfaceMesh& mesh_, const std::vector<glm::vec2>& coords_,
                                        ParamCoordsType type_, ParamVizStyle style);

  virtual void buildVertexInfoGUI(size_t vInd) override;
  virtual std::string niceName() override;


protected:
  virtual void fillCoordBuffers(render::ShaderProgram& p) override;
};

} // namespace polyscope

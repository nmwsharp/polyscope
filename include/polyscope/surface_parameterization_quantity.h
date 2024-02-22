// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/parameterization_quantity.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"


namespace polyscope {

// forward declarations
class CurveNetwork;

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================


class SurfaceParameterizationQuantity : public SurfaceMeshQuantity,
                                        public ParameterizationQuantity<SurfaceParameterizationQuantity> {

public:
  SurfaceParameterizationQuantity(std::string name, SurfaceMesh& mesh_, const std::vector<glm::vec2>& coords_,
                                  MeshElement definedOn, ParamCoordsType type_, ParamVizStyle style_);

  virtual void draw() override;
  virtual void refresh() override;
  virtual void buildCustomUI() override;

  const MeshElement definedOn;

  // Set islands labels. Technically, this data is just any categorical integer labels per-face of the mesh.
  // The intended use is to label islands (connected components in parameterization space) of the UV map.
  // When style == CHECKER_ISLANDS, these will be use to visualize the islands with different colors.
  template <typename V>
  void setIslandLabels(const V& newIslandLabels);

  CurveNetwork* createCurveNetworkFromSeams(std::string structureName = "");

protected:
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  void createProgram();
  size_t nFaces(); // works around an incomplete def of the parent mesh
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


// == template implementations

template <typename V>
void SurfaceParameterizationQuantity::setIslandLabels(const V& newIslandLabels) {
  validateSize(newIslandLabels, this->nFaces(), "scalar quantity " + quantity.name);
  islandLabels.data = standardizeArray<float, V>(newIslandLabels);
  islandLabels.markHostBufferUpdated();
  islandLabelsPopulated = true;
}

} // namespace polyscope

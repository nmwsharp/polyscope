// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/surface_parameterization_enums.h"


namespace polyscope {


// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================


class SurfaceParameterizationQuantity : public SurfaceMeshQuantity {

public:
  SurfaceParameterizationQuantity(std::string name, ParamCoordsType type_, ParamVizStyle style, SurfaceMesh& mesh_);

  void draw() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;


  // === Members
  const ParamCoordsType coordsType;

  // === Viz stuff
  // to keep things simple, has settings for all of the viz styles, even though not all are used at all times


  // === Getters and setters for visualization options

  // What visualization scheme to use
  SurfaceParameterizationQuantity* setStyle(ParamVizStyle newStyle);
  ParamVizStyle getStyle();

  // Colors for checkers
  SurfaceParameterizationQuantity* setCheckerColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getCheckerColors();

  // Colors for checkers
  SurfaceParameterizationQuantity* setGridColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getGridColors();

  // The size of checkers / stripes
  SurfaceParameterizationQuantity* setCheckerSize(double newVal);
  double getCheckerSize();

  // Color map for radial visualization
  SurfaceParameterizationQuantity* setColorMap(std::string val);
  std::string getColorMap();

  // Darkness for checkers (etc)
  SurfaceParameterizationQuantity* setAltDarkness(double newVal);
  double getAltDarkness();
    

protected:
  // === Visualiztion options
  PersistentValue<float> checkerSize;
  PersistentValue<ParamVizStyle> vizStyle;
  PersistentValue<glm::vec3> checkColor1, checkColor2;           // for checker (two colors to use)
  PersistentValue<glm::vec3> gridLineColor, gridBackgroundColor; // for GRID (two colors to use)
  PersistentValue<float> altDarkness;

  PersistentValue<std::string> cMap;
  float localRot = 0.; // for LOCAL (angular shift, in radians)
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  void createProgram();
  void setProgramUniforms(render::ShaderProgram& program);
  virtual void fillColorBuffers(render::ShaderProgram& p) = 0;
};


// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================

class SurfaceCornerParameterizationQuantity : public SurfaceParameterizationQuantity {

public:
  SurfaceCornerParameterizationQuantity(std::string name, std::vector<glm::vec2> values_, ParamCoordsType type_,
                                        ParamVizStyle style, SurfaceMesh& mesh_);

  virtual void buildHalfedgeInfoGUI(size_t heInd) override;
  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec2> coords; // on corners

protected:
  virtual void fillColorBuffers(render::ShaderProgram& p) override;
};


// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================

class SurfaceVertexParameterizationQuantity : public SurfaceParameterizationQuantity {
public:
  SurfaceVertexParameterizationQuantity(std::string name, std::vector<glm::vec2> values_, ParamCoordsType type_,
                                        ParamVizStyle style, SurfaceMesh& mesh_);

  virtual void buildVertexInfoGUI(size_t vInd) override;
  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec2> coords; // on vertices

protected:
  virtual void fillColorBuffers(render::ShaderProgram& p) override;
};

} // namespace polyscope

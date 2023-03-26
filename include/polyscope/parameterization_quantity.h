// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"

namespace polyscope {

// Encapsulates logic which is common to all scalar quantities

template <typename QuantityT>
class ParameterizationQuantity {
public:
  ParameterizationQuantity(QuantityT& quantity, const std::vector<glm::vec2>& coords_, ParamCoordsType type_,
                           ParamVizStyle style_);

  void buildParameterizationUI();
  virtual void buildParameterizationOptionsUI(); // called inside of an options menu

  template <class V>
  void updateCoords(const V& newCoords);

  // === Members
  QuantityT& quantity;


  // Wrapper around the actual buffer of scalar data stored in the class.
  // Interaction with the data (updating it on CPU or GPU side, accessing it, etc) happens through this wrapper.
  render::ManagedBuffer<glm::vec2> coords;

  const ParamCoordsType coordsType;

  // === Get/set visualization parameters

  // The color map

  // What visualization scheme to use
  QuantityT* setStyle(ParamVizStyle newStyle);
  ParamVizStyle getStyle();

  // Colors for checkers
  QuantityT* setCheckerColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getCheckerColors();

  // Colors for checkers
  QuantityT* setGridColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getGridColors();

  // The size of checkers / stripes
  QuantityT* setCheckerSize(double newVal);
  double getCheckerSize();

  // Color map for radial visualization
  QuantityT* setColorMap(std::string val);
  std::string getColorMap();

  // Darkness for checkers (etc)
  QuantityT* setAltDarkness(double newVal);
  double getAltDarkness();

  // === Helpers for rendering
  std::vector<std::string> addParameterizationRules(std::vector<std::string> rules);
  void fillParameterizationBuffers(render::ShaderProgram& p);
  void setParameterizationUniforms(render::ShaderProgram& p);

protected:
  // Raw storage for the data. You should only interact with this via the managed buffer above
  std::vector<glm::vec2> coordsData;

  // === Visualization parameters

  // Parameters
  PersistentValue<float> checkerSize;
  PersistentValue<ParamVizStyle> vizStyle;
  PersistentValue<glm::vec3> checkColor1, checkColor2;           // for checker (two colors to use)
  PersistentValue<glm::vec3> gridLineColor, gridBackgroundColor; // for GRID (two colors to use)
  PersistentValue<float> altDarkness;
  PersistentValue<std::string> cMap;
  float localRot = 0.; // for LOCAL (angular shift, in radians)
};

} // namespace polyscope


#include "polyscope/parameterization_quantity.ipp"

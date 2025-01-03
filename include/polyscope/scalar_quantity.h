// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"

namespace polyscope {

// Encapsulates logic which is common to all scalar quantities

template <typename QuantityT>
class ScalarQuantity {
public:
  ScalarQuantity(QuantityT& quantity, const std::vector<float>& values, DataType dataType);

  // Build the ImGUI UIs for scalars
  void buildScalarUI();
  virtual void buildScalarOptionsUI(); // called inside of an options menu

  // Add rules to rendering programs for scalars
  std::vector<std::string> addScalarRules(std::vector<std::string> rules);

  // Set uniforms in rendering programs for scalars
  void setScalarUniforms(render::ShaderProgram& p);

  template <class V>
  void updateData(const V& newValues);

  // === Members
  QuantityT& quantity;

  // Wrapper around the actual buffer of scalar data stored in the class.
  // Interaction with the data (updating it on CPU or GPU side, accessing it, etc) happens through this wrapper.
  render::ManagedBuffer<float> values;

  // === Get/set visualization parameters

  // The color map
  QuantityT* setColorMap(std::string val);
  std::string getColorMap();

  // Data limits mapped in to colormap
  QuantityT* setMapRange(std::pair<double, double> val);
  std::pair<double, double> getMapRange();
  QuantityT* resetMapRange(); // reset to full range
  std::pair<double, double> getDataRange();

  // Isolines
  // NOTE there's a name typo, errant `s` in isolinesEnabled (leaving to avoid breaking change)
  QuantityT* setIsolinesEnabled(bool newEnabled);
  bool getIsolinesEnabled();
  QuantityT* setIsolineStyle(IsolineStyle val);
  IsolineStyle getIsolineStyle();
  QuantityT* setIsolinePeriod(double size, bool isRelative);
  double getIsolinePeriod();
  QuantityT* setIsolineDarkness(double val);
  double getIsolineDarkness();
  QuantityT* setIsolineContourThickness(double val);
  double getIsolineContourThickness();

  // Old / depracted methods kept for compatability
  QuantityT* setIsolineWidth(double size, bool isRelative);
  double getIsolineWidth();

protected:
  std::vector<float> valuesData;
  const DataType dataType;

  // === Visualization parameters

  // Affine data maps and limits
  std::pair<double, double> dataRange;
  PersistentValue<float> vizRangeMin;
  PersistentValue<float> vizRangeMax;
  Histogram hist;

  // Parameters
  PersistentValue<std::string> cMap;
  PersistentValue<bool> isolinesEnabled;
  PersistentValue<IsolineStyle> isolineStyle;
  PersistentValue<ScaledValue<float>> isolinePeriod;
  PersistentValue<float> isolineDarkness;
  PersistentValue<float> isolineContourThickness;
};

} // namespace polyscope


#include "polyscope/scalar_quantity.ipp"

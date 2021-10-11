namespace polyscope {

template <typename QuantityT>
ScalarQuantity<QuantityT>::ScalarQuantity(QuantityT& quantity_, const std::vector<double>& values_, DataType dataType_)
    : quantity(quantity_), values(values_), dataType(dataType_), dataRange(robustMinMax(values, 1e-5)),
      cMap(quantity.name + "#cmap", defaultColorMap(dataType)),
      isolinesEnabled(quantity.name + "#isolinesEnabled", false),
      isolineWidth(quantity.name + "#isolineWidth", absoluteValue((dataRange.second - dataRange.first) * 0.02)),
      isolineDarkness(quantity.name + "#isolineDarkness", 0.7),
      contoursEnabled(quantity.name + "#contoursEnabled", false),
      contourFrequency(quantity.name + "#contourFrequency", absoluteValue((dataRange.second - dataRange.first) * 0.3)),
      contourThickness(quantity.name + "#contourThickness", 0.2),
      contourDarkness(quantity.name + "#contourThickness", 0.5)

{
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values);
  resetMapRange();
}

template <typename QuantityT>
void ScalarQuantity<QuantityT>::buildScalarUI() {

  if (render::buildColormapSelector(cMap.get())) {
    quantity.refresh();
    hist.updateColormap(cMap.get());
    setColorMap(getColorMap());
  }

  // Reset button
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    resetMapRange();
  }

  // Draw the histogram of values
  hist.colormapRange = vizRange;
  hist.buildUI();

  // Data range
  // Note: %g specifiers are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRange.first, &vizRange.second, (dataRange.second - dataRange.first) / 100.,
                             dataRange.first, dataRange.second, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
      ImGui::DragFloatRange2("##range_symmetric", &vizRange.first, &vizRange.second, absRange / 100., -absRange,
                             absRange, "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRange.first, &vizRange.second, vizRange.second / 100., 0.0,
                             dataRange.second, "Min: %.3e", "Max: %.3e");
    } break;
    }
  }

  // Isolines
  if (isolinesEnabled.get()) {
    ImGui::PushItemWidth(100);

    // Isoline width
    ImGui::TextUnformatted("Isoline width");
    ImGui::SameLine();
    if (isolineWidth.get().isRelative()) {
      if (ImGui::DragFloat("##Isoline width relative", isolineWidth.get().getValuePtr(), .001, 0.0001, 1.0, "%.4f",
                           2.0)) {
        isolineWidth.manuallyChanged();
        requestRedraw();
      }
    } else {
      float scaleWidth = dataRange.second - dataRange.first;
      if (ImGui::DragFloat("##Isoline width absolute", isolineWidth.get().getValuePtr(), scaleWidth / 1000, 0.,
                           scaleWidth, "%.4f", 2.0)) {
        isolineWidth.manuallyChanged();
        requestRedraw();
      }
    }

    // Isoline darkness
    ImGui::TextUnformatted("Isoline darkness");
    ImGui::SameLine();
    if (ImGui::DragFloat("##Isoline darkness", &isolineDarkness.get(), 0.01, 0., 1.)) {
      isolineDarkness.manuallyChanged();
      requestRedraw();
    }

    ImGui::PopItemWidth();
  }

  // Contours
  if (contoursEnabled.get()) {
    ImGui::PushItemWidth(100);

    // Contour frequency
    ImGui::TextUnformatted("Contour frequency");
    ImGui::SameLine();
    if (contourFrequency.get().isRelative()) {
      if (ImGui::DragFloat("##Contour frequency relative", contourFrequency.get().getValuePtr(), .001, 0.0001, 1.0, "%.4f",
                           2.0)) {
        contourFrequency.manuallyChanged();
        requestRedraw();
      }
    } else {
      float scaleWidth = dataRange.second - dataRange.first;
      if (ImGui::DragFloat("##Contour frequency absolute", contourFrequency.get().getValuePtr(), scaleWidth / 1000, 0.,
                           scaleWidth, "%.4f", 2.0)) {
        contourFrequency.manuallyChanged();
        requestRedraw();
      }
    }

    // Contour thickness
    ImGui::TextUnformatted("Contour thickness");
    ImGui::SameLine();
    if (ImGui::DragFloat("##Contour thickness", &contourThickness.get(), 0.01, 0., 1.)) {
      contourThickness.manuallyChanged();
      requestRedraw();
    }

    // Contour darkness
    ImGui::TextUnformatted("Contour darkness");
    ImGui::SameLine();
    if (ImGui::DragFloat("##Contour darkness", &contourDarkness.get(), 0.01, 0., 1.)) {
      contourDarkness.manuallyChanged();
      requestRedraw();
    }

    ImGui::PopItemWidth();
  }

}

template <typename QuantityT>
void ScalarQuantity<QuantityT>::buildScalarOptionsUI() {
  if (ImGui::MenuItem("Reset colormap range")) resetMapRange();
  if (ImGui::MenuItem("Enable isolines", NULL, isolinesEnabled.get())) setIsolinesEnabled(!isolinesEnabled.get());
  if (ImGui::MenuItem("Enable contours", NULL, contoursEnabled.get())) setContoursEnabled(!contoursEnabled.get());
}

template <typename QuantityT>
std::vector<std::string> ScalarQuantity<QuantityT>::addScalarRules(std::vector<std::string> rules) {
  rules.push_back("SHADE_COLORMAP_VALUE");
  if (isolinesEnabled.get()) {
    rules.push_back("ISOLINE_STRIPE_VALUECOLOR");
  }
  if (contoursEnabled.get()) {
    rules.push_back("CONTOUR_VALUECOLOR");
  }
  return rules;
}


template <typename QuantityT>
void ScalarQuantity<QuantityT>::setScalarUniforms(render::ShaderProgram& p) {
  p.setUniform("u_rangeLow", vizRange.first);
  p.setUniform("u_rangeHigh", vizRange.second);

  if (isolinesEnabled.get()) {
    p.setUniform("u_modLen", getIsolineWidth());
    p.setUniform("u_modDarkness", getIsolineDarkness());
  }

  if (contoursEnabled.get()) {
    p.setUniform("u_modFrequency", getContourFrequency());
    p.setUniform("u_modThickness", getContourThickness());
    p.setUniform("u_modDarkness", getContourDarkness());
  }
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::resetMapRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRange = dataRange;
    break;
  case DataType::SYMMETRIC: {
    double absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
    vizRange = std::make_pair(-absRange, absRange);
  } break;
  case DataType::MAGNITUDE:
    vizRange = std::make_pair(0., dataRange.second);
    break;
  }

  requestRedraw();
  return &quantity;
}


template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setColorMap(std::string val) {
  cMap = val;
  hist.updateColormap(cMap.get());
  quantity.refresh();
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
std::string ScalarQuantity<QuantityT>::getColorMap() {
  return cMap.get();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setMapRange(std::pair<double, double> val) {
  vizRange = val;
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
std::pair<double, double> ScalarQuantity<QuantityT>::getMapRange() {
  return vizRange;
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setIsolineWidth(double size, bool isRelative) {
  isolineWidth = ScaledValue<float>(size, isRelative);
  if (!isolinesEnabled.get()) {
    setIsolinesEnabled(true);
  }
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double ScalarQuantity<QuantityT>::getIsolineWidth() {
  return isolineWidth.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setIsolineDarkness(double val) {
  isolineDarkness = val;
  if (!isolinesEnabled.get()) {
    setIsolinesEnabled(true);
  }
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double ScalarQuantity<QuantityT>::getIsolineDarkness() {
  return isolineDarkness.get();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setIsolinesEnabled(bool newEnabled) {
  isolinesEnabled = newEnabled;
  quantity.refresh();
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
bool ScalarQuantity<QuantityT>::getIsolinesEnabled() {
  return isolinesEnabled.get();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setContourFrequency(double size, bool isRelative) {
  contourFrequency = ScaledValue<float>(size, isRelative);
  if (!contoursEnabled.get()) {
    setContoursEnabled(true);
  }
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double ScalarQuantity<QuantityT>::getContourFrequency() {
  return contourFrequency.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setContourThickness(double val) {
  contourThickness = val;
  if (!contoursEnabled.get()) {
    setContoursEnabled(true);
  }
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double ScalarQuantity<QuantityT>::getContourThickness() {
  return contourThickness.get();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setContourDarkness(double val) {
  contourDarkness = val;
  if (!contoursEnabled.get()) {
    setContoursEnabled(true);
  }
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double ScalarQuantity<QuantityT>::getContourDarkness() {
  return contourDarkness.get();
}

template <typename QuantityT>
QuantityT* ScalarQuantity<QuantityT>::setContoursEnabled(bool newEnabled) {
  contoursEnabled = newEnabled;
  quantity.refresh();
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
bool ScalarQuantity<QuantityT>::getContoursEnabled() {
  return contoursEnabled.get();
}

} // namespace polyscope

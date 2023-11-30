// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "imgui.h"

#include "polyscope/utilities.h"

namespace polyscope {


namespace {
// Helper to name styles
std::string styleName(ParamVizStyle v) {
  switch (v) {
  case ParamVizStyle::CHECKER:
    return "checker";
    break;
  case ParamVizStyle::CHECKER_ISLANDS:
    return "checker islands";
    break;
  case ParamVizStyle::GRID:
    return "grid";
    break;
  case ParamVizStyle::LOCAL_CHECK:
    return "local grid";
    break;
  case ParamVizStyle::LOCAL_RAD:
    return "local dist";
    break;
  }
  exception("broken");
  return "";
}

} // namespace

template <typename QuantityT>
ParameterizationQuantity<QuantityT>::ParameterizationQuantity(QuantityT& quantity_,
                                                              const std::vector<glm::vec2>& coords_,
                                                              ParamCoordsType type_, ParamVizStyle style_)
    : quantity(quantity_),

      // buffers
      coords(&quantity, quantity.uniquePrefix() + "#coords", coordsData),
      islandLabels(&quantity, quantity.uniquePrefix() + "#islandLabels", islandLabelsData), coordsType(type_),
      coordsData(coords_),

      // options
      checkerSize(quantity.uniquePrefix() + "#checkerSize", 0.02),
      vizStyle(quantity.uniquePrefix() + "#vizStyle", style_),
      checkColor1(quantity.uniquePrefix() + "#checkColor1", render::RGB_PINK),
      checkColor2(quantity.uniquePrefix() + "#checkColor2", glm::vec3(.976, .856, .885)),
      gridLineColor(quantity.uniquePrefix() + "#gridLineColor", render::RGB_WHITE),
      gridBackgroundColor(quantity.uniquePrefix() + "#gridBackgroundColor", render::RGB_PINK),
      altDarkness(quantity.uniquePrefix() + "#altDarkness", 0.5), cMap(quantity.uniquePrefix() + "#cMap", "phase")


{}


template <typename QuantityT>
void ParameterizationQuantity<QuantityT>::buildParameterizationUI() {

  ImGui::PushItemWidth(100);

  // Modulo stripey width
  if (ImGui::DragFloat("period", &checkerSize.get(), .001, 0.0001, 1.0, "%.4f",
                       ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
    setCheckerSize(getCheckerSize());
  }


  ImGui::PopItemWidth();

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    ImGui::SameLine();
    if (ImGui::ColorEdit3("##colors2", &checkColor1.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("##colors", &checkColor2.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    break;
  case ParamVizStyle::CHECKER_ISLANDS:
    ImGui::PushItemWidth(100);
    if (ImGui::DragFloat("alt darkness", &altDarkness.get(), 0.01, 0., 1.)) {
      altDarkness.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();
    if (render::buildColormapSelector(cMap.get())) {
      setColorMap(getColorMap());
    }
    break;
  case ParamVizStyle::GRID:
    ImGui::SameLine();
    if (ImGui::ColorEdit3("##base", &gridBackgroundColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("##line", &gridLineColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD: {
    // Angle slider
    ImGui::PushItemWidth(100);
    ImGui::SliderAngle("angle shift", &localRot, -180,
                       180); // displays in degrees, works in radians TODO refresh/update/persist
    if (ImGui::DragFloat("alt darkness", &altDarkness.get(), 0.01, 0., 1.)) {
      altDarkness.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();

    // Set colormap
    if (render::buildColormapSelector(cMap.get())) {
      setColorMap(getColorMap());
    }
  }

  break;
  }
}

template <typename QuantityT>
void ParameterizationQuantity<QuantityT>::buildParameterizationOptionsUI() {

  // Choose viz style
  if (ImGui::BeginMenu("Style")) {
    for (ParamVizStyle s : {ParamVizStyle::CHECKER_ISLANDS, ParamVizStyle::CHECKER, ParamVizStyle::GRID,
                            ParamVizStyle::LOCAL_CHECK, ParamVizStyle::LOCAL_RAD}) {

      if (s == ParamVizStyle::CHECKER_ISLANDS && !haveIslandLabels()) {
        // only allow CHECKER_ISLANDS if we actually have island labels
        continue;
      }

      bool selected = s == getStyle();
      std::string fancyName = styleName(s);
      if (ImGui::MenuItem(fancyName.c_str(), NULL, selected)) {
        setStyle(s);
      }
    }
    ImGui::EndMenu();
  }
}

template <typename QuantityT>
std::vector<std::string> ParameterizationQuantity<QuantityT>::addParameterizationRules(std::vector<std::string> rules) {

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    rules.insert(rules.end(), {"SHADE_CHECKER_VALUE2"});
    break;
  case ParamVizStyle::CHECKER_ISLANDS:
    rules.insert(rules.end(), {"SHADE_CHECKER_CATEGORY"});
    break;
  case ParamVizStyle::GRID:
    rules.insert(rules.end(), {"SHADE_GRID_VALUE2"});
    break;
  case ParamVizStyle::LOCAL_CHECK:
    rules.insert(rules.end(), {"SHADE_COLORMAP_ANGULAR2", "CHECKER_VALUE2COLOR"});
    break;
  case ParamVizStyle::LOCAL_RAD:
    rules.insert(rules.end(), {"SHADE_COLORMAP_ANGULAR2", "SHADEVALUE_MAG_VALUE2", "ISOLINE_STRIPE_VALUECOLOR"});
    break;
  }

  return rules;
}

template <typename QuantityT>
void ParameterizationQuantity<QuantityT>::fillParameterizationBuffers(render::ShaderProgram& p) {
  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    break;
  case ParamVizStyle::CHECKER_ISLANDS:
    p.setTextureFromColormap("t_colormap", cMap.get());
    break;
  case ParamVizStyle::GRID:
    break;
  case ParamVizStyle::LOCAL_CHECK:
    p.setTextureFromColormap("t_colormap", cMap.get());
    break;
  case ParamVizStyle::LOCAL_RAD:
    p.setTextureFromColormap("t_colormap", cMap.get());
    break;
  }
}


template <typename QuantityT>
void ParameterizationQuantity<QuantityT>::setParameterizationUniforms(render::ShaderProgram& p) {

  // Interpretatin of modulo parameter depends on data type
  switch (coordsType) {
  case ParamCoordsType::UNIT:
    p.setUniform("u_modLen", getCheckerSize());
    break;
  case ParamCoordsType::WORLD:
    p.setUniform("u_modLen", getCheckerSize() * state::lengthScale);
    break;
  }

  // Set other uniforms needed
  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    p.setUniform("u_color1", getCheckerColors().first);
    p.setUniform("u_color2", getCheckerColors().second);
    break;
  case ParamVizStyle::CHECKER_ISLANDS:
    p.setUniform("u_modDarkness", getAltDarkness());
    break;
  case ParamVizStyle::GRID:
    p.setUniform("u_gridLineColor", getGridColors().first);
    p.setUniform("u_gridBackgroundColor", getGridColors().second);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD:
    p.setUniform("u_angle", localRot);
    p.setUniform("u_modDarkness", getAltDarkness());
    break;
  }
}

template <typename QuantityT>
template <class V>
void ParameterizationQuantity<QuantityT>::updateCoords(const V& newCoords) {
  validateSize(newCoords, coords.size(), "parameterization quantity " + quantity.name);
  coords.data = standardizeVectorArray<glm::vec2, 2>(newCoords);
  coords.markHostBufferUpdated();
}

template <typename QuantityT>
bool ParameterizationQuantity<QuantityT>::haveIslandLabels() {
  return islandLabelsPopulated;
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setStyle(ParamVizStyle newStyle) {

  if (newStyle == ParamVizStyle::CHECKER_ISLANDS && !haveIslandLabels()) {
    exception("Cannot set parameterization visualization style to 'CHECKER_ISLANDS', no islands have been set");
    newStyle = ParamVizStyle::CHECKER;
  }

  vizStyle = newStyle;
  quantity.refresh();
  requestRedraw();
  return &quantity;
}

template <typename QuantityT>
ParamVizStyle ParameterizationQuantity<QuantityT>::getStyle() {
  return vizStyle.get();
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setCheckerColors(std::pair<glm::vec3, glm::vec3> colors) {
  checkColor1 = colors.first;
  checkColor2 = colors.second;
  requestRedraw();
  return &quantity;
}

template <typename QuantityT>
std::pair<glm::vec3, glm::vec3> ParameterizationQuantity<QuantityT>::getCheckerColors() {
  return std::make_pair(checkColor1.get(), checkColor2.get());
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setGridColors(std::pair<glm::vec3, glm::vec3> colors) {
  gridLineColor = colors.first;
  gridBackgroundColor = colors.second;
  requestRedraw();
  return &quantity;
}

template <typename QuantityT>
std::pair<glm::vec3, glm::vec3> ParameterizationQuantity<QuantityT>::getGridColors() {
  return std::make_pair(gridLineColor.get(), gridBackgroundColor.get());
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setCheckerSize(double newVal) {
  checkerSize = newVal;
  requestRedraw();
  return &quantity;
}

template <typename QuantityT>
double ParameterizationQuantity<QuantityT>::getCheckerSize() {
  return checkerSize.get();
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setColorMap(std::string name) {
  cMap = name;
  quantity.refresh();
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
std::string ParameterizationQuantity<QuantityT>::getColorMap() {
  return cMap.get();
}

template <typename QuantityT>
QuantityT* ParameterizationQuantity<QuantityT>::setAltDarkness(double newVal) {
  altDarkness = newVal;
  requestRedraw();
  return &quantity;
}

template <typename QuantityT>
double ParameterizationQuantity<QuantityT>::getAltDarkness() {
  return altDarkness.get();
}


} // namespace polyscope

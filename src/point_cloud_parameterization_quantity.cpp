// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

PointCloudParameterizationQuantity::PointCloudParameterizationQuantity(std::string name,
                                                                       const std::vector<glm::vec2>& coords_,
                                                                       ParamCoordsType type_, ParamVizStyle style_,
                                                                       PointCloud& cloud_)
    : PointCloudQuantity(name, cloud_, true), coords(coords_), coordsType(type_),
      checkerSize(uniquePrefix() + "#checkerSize", 0.02), vizStyle(uniquePrefix() + "#vizStyle", style_),
      checkColor1(uniquePrefix() + "#checkColor1", render::RGB_PINK),
      checkColor2(uniquePrefix() + "#checkColor2", glm::vec3(.976, .856, .885)),
      gridLineColor(uniquePrefix() + "#gridLineColor", render::RGB_WHITE),
      gridBackgroundColor(uniquePrefix() + "#gridBackgroundColor", render::RGB_PINK),
      altDarkness(uniquePrefix() + "#altDarkness", 0.5), cMap(uniquePrefix() + "#cMap", "phase")

{}


void PointCloudParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  setProgramUniforms(*program);
  parent.setPointCloudUniforms(*program);

  program->draw();
}

void PointCloudParameterizationQuantity::createProgram() {
  // Create the program to draw this quantity

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    program = render::engine->requestShader(
        "RAYCAST_SPHERE", parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_CHECKER_VALUE2"}));
    break;
  case ParamVizStyle::GRID:
    program = render::engine->requestShader(
        "RAYCAST_SPHERE", parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_GRID_VALUE2"}));
    break;
  case ParamVizStyle::LOCAL_CHECK:
    program = render::engine->requestShader(
        "RAYCAST_SPHERE",
        parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2", "CHECKER_VALUE2COLOR"}));
    program->setTextureFromColormap("t_colormap", cMap.get());
    break;
  case ParamVizStyle::LOCAL_RAD:
    program = render::engine->requestShader(
        "RAYCAST_SPHERE", parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2",
                                                     "SHADEVALUE_MAG_VALUE2", "ISOLINE_STRIPE_VALUECOLOR"}));
    program->setTextureFromColormap("t_colormap", cMap.get());
    break;
  }

  // Fill buffers
  parent.fillGeometryBuffers(*program);
  program->setAttribute("a_value2", coords);

  render::engine->setMaterial(*program, parent.getMaterial());
}


// Update range uniforms
void PointCloudParameterizationQuantity::setProgramUniforms(render::ShaderProgram& program) {
  // Interpretatin of modulo parameter depends on data type
  switch (coordsType) {
  case ParamCoordsType::UNIT:
    program.setUniform("u_modLen", getCheckerSize());
    break;
  case ParamCoordsType::WORLD:
    program.setUniform("u_modLen", getCheckerSize() * state::lengthScale);
    break;
  }

  // Set other uniforms needed
  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    program.setUniform("u_color1", getCheckerColors().first);
    program.setUniform("u_color2", getCheckerColors().second);
    break;
  case ParamVizStyle::GRID:
    program.setUniform("u_gridLineColor", getGridColors().first);
    program.setUniform("u_gridBackgroundColor", getGridColors().second);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD:
    program.setUniform("u_angle", localRot);
    program.setUniform("u_modDarkness", getAltDarkness());
    break;
  }
}

namespace {
// Helper to name styles
std::string styleName(ParamVizStyle v) {
  switch (v) {
  case ParamVizStyle::CHECKER:
    return "checker";
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
  throw std::runtime_error("broken");
}

} // namespace

void PointCloudParameterizationQuantity::buildCustomUI() {
  ImGui::PushItemWidth(100);

  ImGui::SameLine(); // put it next to enabled

  // Choose viz style
  if (ImGui::BeginCombo("style", styleName(getStyle()).c_str())) {
    for (ParamVizStyle s :
         {ParamVizStyle::CHECKER, ParamVizStyle::GRID, ParamVizStyle::LOCAL_CHECK, ParamVizStyle::LOCAL_RAD}) {
      if (ImGui::Selectable(styleName(s).c_str(), s == getStyle())) {
        setStyle(s);
      }
    }
    ImGui::EndCombo();
  }


  // Modulo stripey width
  if (ImGui::DragFloat("period", &checkerSize.get(), .001, 0.0001, 1.0, "%.4f", 2.0)) {
    setCheckerSize(getCheckerSize());
  }


  ImGui::PopItemWidth();

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    if (ImGui::ColorEdit3("##colors2", &checkColor1.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("colors", &checkColor2.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    break;
  case ParamVizStyle::GRID:
    if (ImGui::ColorEdit3("base", &gridBackgroundColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("line", &gridLineColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD: {
    // Angle slider
    ImGui::PushItemWidth(100);
    ImGui::SliderAngle("angle shift", &localRot, -180, 180); // displays in degrees, works in radians
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


PointCloudParameterizationQuantity* PointCloudParameterizationQuantity::setStyle(ParamVizStyle newStyle) {
  vizStyle = newStyle;
  program.reset();
  requestRedraw();
  return this;
}

ParamVizStyle PointCloudParameterizationQuantity::getStyle() { return vizStyle.get(); }

PointCloudParameterizationQuantity*
PointCloudParameterizationQuantity::setCheckerColors(std::pair<glm::vec3, glm::vec3> colors) {
  checkColor1 = colors.first;
  checkColor2 = colors.second;
  requestRedraw();
  return this;
}

std::pair<glm::vec3, glm::vec3> PointCloudParameterizationQuantity::getCheckerColors() {
  return std::make_pair(checkColor1.get(), checkColor2.get());
}

PointCloudParameterizationQuantity*
PointCloudParameterizationQuantity::setGridColors(std::pair<glm::vec3, glm::vec3> colors) {
  gridLineColor = colors.first;
  gridBackgroundColor = colors.second;
  requestRedraw();
  return this;
}

std::pair<glm::vec3, glm::vec3> PointCloudParameterizationQuantity::getGridColors() {
  return std::make_pair(gridLineColor.get(), gridBackgroundColor.get());
}

PointCloudParameterizationQuantity* PointCloudParameterizationQuantity::setCheckerSize(double newVal) {
  checkerSize = newVal;
  requestRedraw();
  return this;
}

double PointCloudParameterizationQuantity::getCheckerSize() { return checkerSize.get(); }

PointCloudParameterizationQuantity* PointCloudParameterizationQuantity::setColorMap(std::string name) {
  cMap = name;
  program.reset();
  requestRedraw();
  return this;
}
std::string PointCloudParameterizationQuantity::getColorMap() { return cMap.get(); }

PointCloudParameterizationQuantity* PointCloudParameterizationQuantity::setAltDarkness(double newVal) {
  altDarkness = newVal;
  requestRedraw();
  return this;
}

double PointCloudParameterizationQuantity::getAltDarkness() { return altDarkness.get(); }


void PointCloudParameterizationQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string PointCloudParameterizationQuantity::niceName() { return name + " (point parameterization)"; }

void PointCloudParameterizationQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coords[ind].x, coords[ind].y);
  ImGui::NextColumn();
}


} // namespace polyscope

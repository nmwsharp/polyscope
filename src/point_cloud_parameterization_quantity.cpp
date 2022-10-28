// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

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

{
  if (coords_.size() != parent.nPoints()) {
    polyscope::error("Point cloud param quantity " + name + " does not have same number of values (" +
                     std::to_string(coords_.size()) + ") as point cloud size (" + std::to_string(parent.nPoints()) +
                     ")");
  }
}


void PointCloudParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (pointProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*pointProgram);
  setProgramUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);

  pointProgram->draw();
}

void PointCloudParameterizationQuantity::createProgram() {
  // Create the pointProgram to draw this quantity

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:

    // clang-format off
    pointProgram = render::engine->requestShader(
        parent.getShaderNameForRenderMode(), 
        parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_CHECKER_VALUE2"})
    );
    // clang-format on


    break;
  case ParamVizStyle::GRID:

    // clang-format off
    pointProgram = render::engine->requestShader(
        parent.getShaderNameForRenderMode(), 
        parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_GRID_VALUE2"})
    );
    // clang-format on

    break;
  case ParamVizStyle::LOCAL_CHECK:

    // clang-format off
    pointProgram = render::engine->requestShader(
        parent.getShaderNameForRenderMode(), 
        parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2", "CHECKER_VALUE2COLOR"})
    );
    // clang-format on

    pointProgram->setTextureFromColormap("t_colormap", cMap.get());

    break;
  case ParamVizStyle::LOCAL_RAD:

    // clang-format off
    pointProgram = render::engine->requestShader(
        parent.getShaderNameForRenderMode(), 
        parent.addPointCloudRules({"SPHERE_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2", "SHADEVALUE_MAG_VALUE2", "ISOLINE_STRIPE_VALUECOLOR"})
    );
    // clang-format on

    pointProgram->setTextureFromColormap("t_colormap", cMap.get());

    break;
  }

  parent.setPointProgramGeometryBuffers(*pointProgram);
  pointProgram->setAttribute("a_value2", getCoordRenderBuffer());

  render::engine->setMaterial(*pointProgram, parent.getMaterial());
}

void PointCloudParameterizationQuantity::ensureRenderBuffersFilled(bool forceRefill) {

  // ## create the buffers if they don't already exist

  bool createdBuffer = false;
  if (!coordRenderBuffer) {
    coordRenderBuffer = render::engine->generateAttributeBuffer(RenderDataType::Vector2Float);
    createdBuffer = true;
  }

  // If the buffers already existed (and thus are presumably filled), quick-out. Otherwise, fill the buffers.
  if (createdBuffer || forceRefill) {
    coordRenderBuffer->setData(coords);
  }
}

void PointCloudParameterizationQuantity::dataUpdated() {
  ensureRenderBuffersFilled(false);
  requestRedraw();
}

std::shared_ptr<render::AttributeBuffer> PointCloudParameterizationQuantity::getCoordRenderBuffer() {
  ensureRenderBuffersFilled();
  return coordRenderBuffer;
}

uint32_t PointCloudParameterizationQuantity::getCoordBufferID() {
  ensureRenderBuffersFilled();
  return coordRenderBuffer->getNativeBufferID();
}

void PointCloudParameterizationQuantity::bufferDataExternallyUpdated() { requestRedraw(); }


// Update range uniforms
void PointCloudParameterizationQuantity::setProgramUniforms(render::ShaderProgram& pointProgram) {
  // Interpretatin of modulo parameter depends on data type
  switch (coordsType) {
  case ParamCoordsType::UNIT:
    pointProgram.setUniform("u_modLen", getCheckerSize());
    break;
  case ParamCoordsType::WORLD:
    pointProgram.setUniform("u_modLen", getCheckerSize() * state::lengthScale);
    break;
  }

  // Set other uniforms needed
  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    pointProgram.setUniform("u_color1", getCheckerColors().first);
    pointProgram.setUniform("u_color2", getCheckerColors().second);
    break;
  case ParamVizStyle::GRID:
    pointProgram.setUniform("u_gridLineColor", getGridColors().first);
    pointProgram.setUniform("u_gridBackgroundColor", getGridColors().second);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD:
    pointProgram.setUniform("u_angle", localRot);
    pointProgram.setUniform("u_modDarkness", getAltDarkness());
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
  if (ImGui::DragFloat("period", &checkerSize.get(), .001, 0.0001, 1.0, "%.4f",
                       ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
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
  pointProgram.reset();
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
  pointProgram.reset();
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
  pointProgram.reset();
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

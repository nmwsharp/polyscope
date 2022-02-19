// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud.h"

#include "polyscope/file_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string PointCloud::structureTypeName = "Point Cloud";

// Constructor
PointCloud::PointCloud(std::string name, std::vector<glm::vec3> points_)
    : QuantityStructure<PointCloud>(name, structureTypeName), points(std::move(points_)),
      pointRenderMode(uniquePrefix() + "#pointRenderMode", "sphere"),
      pointColor(uniquePrefix() + "#pointColor", getNextUniqueColor()),
      pointRadius(uniquePrefix() + "#pointRadius", relativeValue(0.005)),
      material(uniquePrefix() + "#material", "clay") {
  cullWholeElements.setPassive(true);
  updateObjectSpaceBounds();
}

// Helper to set uniforms
void PointCloud::setPointCloudUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  if (getPointRenderMode() == PointRenderMode::Sphere) {
    p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
    p.setUniform("u_viewport", render::engine->getCurrentViewport());
  }

  if (pointRadiusQuantityName != "" && !pointRadiusQuantityAutoscale) {
    // special case: ignore radius uniform
    p.setUniform("u_pointRadius", 1.);
  } else {
    // common case
    p.setUniform("u_pointRadius", pointRadius.get().asAbsolute());
  }
}

void PointCloud::draw() {
  if (!isEnabled()) {
    return;
  }

  // If the user creates a very big point cloud using sphere mode, print a warning
  // (this warning is only printed once, and only if verbosity is high enough)
  if (points.size() > 500000 && getPointRenderMode() == PointRenderMode::Sphere &&
      !internal::pointCloudEfficiencyWarningReported && options::verbosity > 1) {
    info("To render large point clouds efficiently, set their render mode to 'quad' instead of 'sphere'. (disable "
         "these warnings by setting Polyscope's verbosity < 2)");
    internal::pointCloudEfficiencyWarningReported = true;
  }


  // If there is no dominant quantity, then this class is responsible for drawing points
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    if (program == nullptr) {
      prepare();
    }

    // Set program uniforms
    setStructureUniforms(*program);
    setPointCloudUniforms(*program);
    program->setUniform("u_baseColor", pointColor.get());

    // Draw the actual point cloud
    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
}

void PointCloud::drawPick() {
  if (!isEnabled()) {
    return;
  }

  // Ensure we have prepared buffers
  if (pickProgram == nullptr) {
    preparePick();
  }

  // Set uniforms
  setStructureUniforms(*pickProgram);
  setPointCloudUniforms(*pickProgram);

  pickProgram->draw();
}

void PointCloud::prepare() {
  // It not quantity is coloring the points, draw with a default color
  if (dominantQuantity != nullptr) {
    return;
  }

  program = render::engine->requestShader(getShaderNameForRenderMode(), addPointCloudRules({"SHADE_BASECOLOR"}));
  render::engine->setMaterial(*program, material.get());

  // Fill out the geometry data for the program
  fillGeometryBuffers(*program);
}

void PointCloud::preparePick() {

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  pickProgram =
      render::engine->requestShader(getShaderNameForRenderMode(), addPointCloudRules({"SPHERE_PROPAGATE_COLOR"}, true),
                                    render::ShaderReplacementDefaults::Pick);

  // Fill color buffer with packed point indices
  std::vector<glm::vec3> pickColors;
  for (size_t i = pickStart; i < pickStart + pickCount; i++) {
    glm::vec3 val = pick::indToVec(i);
    pickColors.push_back(pick::indToVec(i));
  }

  // Store data in buffers
  fillGeometryBuffers(*pickProgram);
  pickProgram->setAttribute("a_color", pickColors);
}

std::string PointCloud::getShaderNameForRenderMode() {
  if (getPointRenderMode() == PointRenderMode::Sphere)
    return "RAYCAST_SPHERE";
  else if (getPointRenderMode() == PointRenderMode::Quad)
    return "POINT_QUAD";
  return "ERROR";
}


std::vector<std::string> PointCloud::addPointCloudRules(std::vector<std::string> initRules, bool withPointCloud) {
  initRules = addStructureRules(initRules);
  if (withPointCloud) {
    if (pointRadiusQuantityName != "") {
      initRules.push_back("SPHERE_VARIABLE_SIZE");
    }
    if (wantsCullPosition()) {
      if (getPointRenderMode() == PointRenderMode::Sphere)
        initRules.push_back("SPHERE_CULLPOS_FROM_CENTER");
      else if (getPointRenderMode() == PointRenderMode::Quad)
        initRules.push_back("SPHERE_CULLPOS_FROM_CENTER_QUAD");
    }
  }
  return initRules;
}

// helper
std::vector<double> PointCloud::resolvePointRadiusQuantity() {
  PointCloudScalarQuantity* sizeScalarQ = nullptr;
  PointCloudQuantity* sizeQ = getQuantity(pointRadiusQuantityName);
  if (sizeQ != nullptr) {
    sizeScalarQ = dynamic_cast<PointCloudScalarQuantity*>(sizeQ);
    if (sizeScalarQ == nullptr) {
      polyscope::error("Cannot populate point size from quantity [" + name + "], it is not a scalar quantity");
    }
  } else {
    polyscope::error("Cannot populate point size from quantity [" + name + "], it does not exist");
  }

  std::vector<double> sizes;
  if (sizeScalarQ == nullptr) {
    // we failed to resolve above; populate with dummy data so we can continue processing
    std::vector<double> ones(nPoints(), 1.);
    sizes = ones;
  } else {
    sizes = sizeScalarQ->values;
  }

  // clamp to nonnegative and autoscale (if requested)
  double max = 0;
  for (double& x : sizes) {
    if (!(x > 0)) x = 0; // ensure all nonnegative
    max = std::fmax(max, x);
  }
  if (max == 0) max = 1e-6;
  if (pointRadiusQuantityAutoscale) {
    for (double& x : sizes) {
      x /= max;
    }
  }

  return sizes;
}

void PointCloud::fillGeometryBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_position", points);

  if (pointRadiusQuantityName != "") {
    // Resolve the quantity
    std::vector<double> pointRadiusQuantityVals = resolvePointRadiusQuantity();
    p.setAttribute("a_pointRadius", pointRadiusQuantityVals);
  }
}

void PointCloud::geometryChanged() {
  if (program) 
  {
    fillGeometryBuffers(*program);
  }
  if (pickProgram) 
  {
    fillGeometryBuffers(*pickProgram);
  }
  requestRedraw();
  QuantityStructure<PointCloud>::refresh();
}

void PointCloud::buildPickUI(size_t localPickID) {

  ImGui::TextUnformatted(("#" + std::to_string(localPickID) + "  ").c_str());
  ImGui::SameLine();
  ImGui::TextUnformatted(to_string(points[localPickID]).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildPickUI(localPickID);
  }

  ImGui::Indent(-20.);
}

void PointCloud::buildCustomUI() {
  ImGui::Text("# points: %lld", static_cast<long long int>(points.size()));
  if (ImGui::ColorEdit3("Point color", &pointColor.get()[0], ImGuiColorEditFlags_NoInputs)) {
    setPointColor(getPointColor());
  }
  ImGui::SameLine();
  ImGui::PushItemWidth(70);
  if (ImGui::SliderFloat("Radius", pointRadius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    pointRadius.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}

void PointCloud::buildCustomOptionsUI() {


  if (ImGui::BeginMenu("Point Render Mode")) {

    for (const PointRenderMode& m : {PointRenderMode::Sphere, PointRenderMode::Quad}) {
      bool selected = (m == getPointRenderMode());
      std::string fancyName;
      switch (m) {
      case PointRenderMode::Sphere:
        fancyName = "sphere (pretty)";
        break;
      case PointRenderMode::Quad:
        fancyName = "quad (fast)";
        break;
      }
      if (ImGui::MenuItem(fancyName.c_str(), NULL, selected)) {
        setPointRenderMode(m);
      }
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Variable Radius")) {

    if (ImGui::MenuItem("none", nullptr, pointRadiusQuantityName == "")) clearPointRadiusQuantity();
    ImGui::Separator();

    for (auto& q : quantities) {
      PointCloudScalarQuantity* scalarQ = dynamic_cast<PointCloudScalarQuantity*>(q.second.get());
      if (scalarQ != nullptr) {
        if (ImGui::MenuItem(scalarQ->name.c_str(), nullptr, pointRadiusQuantityName == scalarQ->name))
          setPointRadiusQuantity(scalarQ);
      }
    }

    ImGui::EndMenu();
  }

  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}

void PointCloud::updateObjectSpaceBounds() {

  // bounding box
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  for (const glm::vec3& p : points) {
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }
  objectSpaceBoundingBox = std::make_tuple(min, max);

  // length scale, as twice the radius from the center of the bounding box
  glm::vec3 center = 0.5f * (min + max);
  float lengthScale = 0.0;
  for (const glm::vec3& p : points) {
    lengthScale = std::max(lengthScale, glm::length2(p - center));
  }
  objectSpaceLengthScale = 2 * std::sqrt(lengthScale);
}


std::string PointCloud::typeName() { return structureTypeName; }


void PointCloud::refresh() {
  program.reset();
  pickProgram.reset();
  QuantityStructure<PointCloud>::refresh(); // call base class version, which refreshes quantities
}


// === Set point size from a scalar quantity
void PointCloud::setPointRadiusQuantity(PointCloudScalarQuantity* quantity, bool autoScale) {
  setPointRadiusQuantity(quantity->name, autoScale);
}

void PointCloud::setPointRadiusQuantity(std::string name, bool autoScale) {
  pointRadiusQuantityName = name;
  pointRadiusQuantityAutoscale = autoScale;

  resolvePointRadiusQuantity(); // do it once, just so we fail fast if it doesn't exist

  refresh(); // TODO this is a bit overkill
}

void PointCloud::clearPointRadiusQuantity() {
  pointRadiusQuantityName = "";
  refresh();
}

// === Quantities

// Quantity default methods
PointCloudQuantity::PointCloudQuantity(std::string name_, PointCloud& pointCloud_, bool dominates_)
    : Quantity<PointCloud>(name_, pointCloud_, dominates_) {}


void PointCloudQuantity::buildInfoGUI(size_t pointInd) {}

// === Quantity adders


PointCloudColorQuantity* PointCloud::addColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors) {
  PointCloudColorQuantity* q = new PointCloudColorQuantity(name, colors, *this);
  addQuantity(q);
  return q;
}

PointCloudScalarQuantity* PointCloud::addScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                            DataType type) {
  PointCloudScalarQuantity* q = new PointCloudScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

PointCloudParameterizationQuantity* PointCloud::addParameterizationQuantityImpl(std::string name,
                                                                                const std::vector<glm::vec2>& param,
                                                                                ParamCoordsType type) {
  PointCloudParameterizationQuantity* q =
      new PointCloudParameterizationQuantity(name, param, type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);
  return q;
}

PointCloudParameterizationQuantity*
PointCloud::addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& param,
                                                 ParamCoordsType type) {
  PointCloudParameterizationQuantity* q =
      new PointCloudParameterizationQuantity(name, param, type, ParamVizStyle::LOCAL_CHECK, *this);
  addQuantity(q);
  return q;
}

PointCloudVectorQuantity* PointCloud::addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors,
                                                            VectorType vectorType) {
  PointCloudVectorQuantity* q = new PointCloudVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

PointCloud* PointCloud::setPointRenderMode(PointRenderMode newVal) {
  switch (newVal) {
  case PointRenderMode::Sphere:
    pointRenderMode = "sphere";
    break;
  case PointRenderMode::Quad:
    pointRenderMode = "quad";
    break;
  }
  refresh();
  polyscope::requestRedraw();
  return this;
}
PointRenderMode PointCloud::getPointRenderMode() {
  // The point render mode is stored as string internally to simplify persistent value handling
  if (pointRenderMode.get() == "sphere")
    return PointRenderMode::Sphere;
  else if (pointRenderMode.get() == "quad")
    return PointRenderMode::Quad;
  return PointRenderMode::Sphere; // should never happen
}

PointCloud* PointCloud::setPointColor(glm::vec3 newVal) {
  pointColor = newVal;
  polyscope::requestRedraw();
  return this;
}
glm::vec3 PointCloud::getPointColor() { return pointColor.get(); }

PointCloud* PointCloud::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string PointCloud::getMaterial() { return material.get(); }

PointCloud* PointCloud::setPointRadius(double newVal, bool isRelative) {
  pointRadius = ScaledValue<float>(newVal, isRelative);
  polyscope::requestRedraw();
  return this;
}
double PointCloud::getPointRadius() { return pointRadius.get().asAbsolute(); }

} // namespace polyscope

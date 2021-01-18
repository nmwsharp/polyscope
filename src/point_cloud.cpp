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

using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string PointCloud::structureTypeName = "Point Cloud";

// Constructor
PointCloud::PointCloud(std::string name, std::vector<glm::vec3> points_)
    : QuantityStructure<PointCloud>(name, structureTypeName), points(std::move(points_)),
      pointColor(uniquePrefix() + "#pointColor", getNextUniqueColor()),
      pointRadius(uniquePrefix() + "#pointRadius", relativeValue(0.005)),
      material(uniquePrefix() + "#material", "clay") {}


// Helper to set uniforms
void PointCloud::setPointCloudUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());

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

  // If there is no dominant quantity, then this class is responsible for drawing points
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    if (program == nullptr) {
      prepare();
    }

    // Set program uniforms
    setTransformUniforms(*program);
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
  setTransformUniforms(*pickProgram);
  setPointCloudUniforms(*pickProgram);

  pickProgram->draw();
}

void PointCloud::prepare() {
  // It not quantity is coloring the points, draw with a default color
  if (dominantQuantity != nullptr) {
    return;
  }

  program = render::engine->requestShader("RAYCAST_SPHERE", addStructureRules({"SHADE_BASECOLOR"}));
  render::engine->setMaterial(*program, material.get());

  // Fill out the geometry data for the program
  fillGeometryBuffers(*program);
}

void PointCloud::preparePick() {

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  pickProgram = render::engine->requestShader("RAYCAST_SPHERE", addStructureRules({"SPHERE_PROPAGATE_COLOR"}),
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


std::vector<std::string> PointCloud::addStructureRules(std::vector<std::string> initRules) {
  if (pointRadiusQuantityName != "") {
    initRules.push_back("SPHERE_VARIABLE_SIZE");
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

void PointCloud::geometryChanged() { refresh(); }

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

double PointCloud::lengthScale() {
  // TODO cache

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (glm::vec3& rawP : points) {
    glm::vec3 p = glm::vec3(objectTransform.get() * glm::vec4(rawP, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> PointCloud::boundingBox() {

  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3& rawP : points) {
    glm::vec3 p = glm::vec3(objectTransform.get() * glm::vec4(rawP, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
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

PointCloudParameterizationQuantity* PointCloud::addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& param, ParamCoordsType type) {
  PointCloudParameterizationQuantity* q = new PointCloudParameterizationQuantity(name, param, type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);
  return q;
}

PointCloudParameterizationQuantity* PointCloud::addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& param, ParamCoordsType type) {
  PointCloudParameterizationQuantity* q = new PointCloudParameterizationQuantity(name, param, type, ParamVizStyle::LOCAL_CHECK, *this);
  addQuantity(q);
  return q;
}

PointCloudVectorQuantity* PointCloud::addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors,
                                                            VectorType vectorType) {
  PointCloudVectorQuantity* q = new PointCloudVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

PointCloud* PointCloud::setPointColor(glm::vec3 newVal) {
  pointColor = newVal;
  polyscope::requestRedraw();
  return this;
}
glm::vec3 PointCloud::getPointColor() { return pointColor.get(); }

PointCloud* PointCloud::setMaterial(std::string m) {
  material = m;
  geometryChanged(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
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

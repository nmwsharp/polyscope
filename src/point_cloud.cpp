// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud.h"

#include "polyscope/file_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"

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
  p.setUniform("u_pointRadius", pointRadius.get().asAbsolute());
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
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

  program = render::engine->generateShaderProgram(
      {render::SPHERE_VERT_SHADER, render::SPHERE_BILLBOARD_GEOM_SHADER, render::SPHERE_BILLBOARD_FRAG_SHADER},
      DrawMode::Points);

  render::engine->setMaterial(*program, material.get());

  // Fill out the geometry data for the program
  program->setAttribute("a_position", points);
}

void PointCloud::preparePick() {

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  pickProgram = render::engine->generateShaderProgram({render::SPHERE_COLOR_VERT_SHADER,
                                                       render::SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                                       render::SPHERE_COLOR_PLAIN_BILLBOARD_FRAG_SHADER},
                                                      DrawMode::Points);

  // Fill color buffer with packed point indices
  std::vector<glm::vec3> pickColors;
  for (size_t i = pickStart; i < pickStart + pickCount; i++) {
    glm::vec3 val = pick::indToVec(i);
    pickColors.push_back(pick::indToVec(i));
  }


  // Store data in buffers
  pickProgram->setAttribute("a_position", points);
  pickProgram->setAttribute("a_color", pickColors);
}

void PointCloud::geometryChanged() {
  program.reset();
  pickProgram.reset();

  for (auto& q : quantities) {
    q.second->geometryChanged();
  }

  requestRedraw();
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
  ImGui::PushItemWidth(100);
  if (ImGui::SliderFloat("Radius", pointRadius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    pointRadius.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}

void PointCloud::buildCustomOptionsUI() {
  if (ImGui::MenuItem("Write points to file")) writePointsToFile();
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
    glm::vec3 p = glm::vec3(objectTransform * glm::vec4(rawP, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> PointCloud::boundingBox() {

  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3& rawP : points) {
    glm::vec3 p = glm::vec3(objectTransform * glm::vec4(rawP, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}


std::string PointCloud::typeName() { return structureTypeName; }

// === Quantities

// Quantity default methods
PointCloudQuantity::PointCloudQuantity(std::string name_, PointCloud& pointCloud_, bool dominates_)
    : Quantity<PointCloud>(name_, pointCloud_, dominates_) {}


void PointCloud::writePointsToFile(std::string filename) {

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  cout << "Writing point cloud " << name << " to file " << filename << endl;

  std::ofstream outFile(filename);
  outFile << "#Polyscope point cloud " << name << endl;
  outFile << "#displayradius " << (pointRadius.get().asAbsolute()) << endl;

  for (size_t i = 0; i < points.size(); i++) {
    outFile << points[i] << endl;
  }

  outFile.close();
}

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

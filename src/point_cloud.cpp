#include "polyscope/point_cloud.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

using namespace geometrycentral;
using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string PointCloud::structureTypeName = "Point Cloud";

PointCloud::PointCloud(std::string name, const std::vector<Vector3>& points_)
    : Structure(name, structureTypeName), points(points_) {

  initialBaseColor = getNextStructureColor();
  pointColor = initialBaseColor;
  colorManager = SubColorManager(initialBaseColor);

  prepare();
  preparePick();
}

PointCloud::~PointCloud() {
  deleteProgram();

  // Delete quantities
  for (auto x : quantities) {
    delete x.second;
  }
}

void PointCloud::deleteProgram() { safeDelete(program); }


// Helper to set uniforms
void PointCloud::setPointCloudUniforms(gl::GLProgram* p, bool withLight, bool withBillboard) {

  glm::mat4 viewMat = view::getCameraViewMatrix();
  p->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  p->setUniform("u_projMatrix", glm::value_ptr(projMat));

  if (withLight) {
    Vector3 eyePos = view::getCameraWorldPosition();
    p->setUniform("u_eye", eyePos);

    p->setUniform("u_lightCenter", state::center);
    p->setUniform("u_lightDist", 2 * state::lengthScale);
  }

  Vector3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);

  if (withBillboard) {
    p->setUniform("u_camZ", lookDir);
    p->setUniform("u_camUp", upDir);
    p->setUniform("u_camRight", rightDir);
  }

  p->setUniform("u_pointRadius", pointRadius * state::lengthScale);
  p->setUniform("u_baseColor", pointColor);
}

void PointCloud::draw() {
  if (!enabled) {
    return;
  }

  if (program == nullptr) {
    prepare();
  }


  // If the current program came from a quantity, allow the quantity to do any necessary per-frame work (like setting
  // uniforms)
  if (activePointQuantity == nullptr) {
    setPointCloudUniforms(program, true, useBillboardSpheres);
  } else {
    setPointCloudUniforms(program, true, activePointQuantity->wantsBillboardUniforms());
    activePointQuantity->setProgramValues(program);
  }

  program->draw();

  // Draw the quantities
  for (auto x : quantities) {
    x.second->draw();
  }
}

void PointCloud::drawPick() {

  if (!enabled) {
    return;
  }

  // Set uniforms
  setPointCloudUniforms(pickProgram, false, true);

  pickProgram->draw();
}

void PointCloud::prepare() {

  // It not quantity is coloring the points, draw with a default color
  if (activePointQuantity == nullptr) {
    if (useBillboardSpheres) {
      program = new gl::GLProgram(&SPHERE_VERT_SHADER, &SPHERE_BILLBOARD_GEOM_SHADER, &SPHERE_BILLBOARD_FRAG_SHADER,
                                  gl::DrawMode::Points);
    } else {
      program = new gl::GLProgram(&SPHERE_VERT_SHADER, &SPHERE_GEOM_SHADER, &SPHERE_FRAG_SHADER, gl::DrawMode::Points);
    }
  }
  // If some quantity is responsible for coloring the points, prepare it
  else {
    program = activePointQuantity->createProgram();
  }

  // Fill out the geometry data for the program
  program->setAttribute("a_position", points);
}

void PointCloud::preparePick() {

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  safeDelete(pickProgram);
  pickProgram = new gl::GLProgram(&SPHERE_COLOR_VERT_SHADER, &SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                  &SPHERE_COLOR_PLAIN_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points);

  // Fill an index buffer
  std::vector<Vector3> pickColors;
  for (size_t i = pickStart; i < pickStart + pickCount; i++) {
    Vector3 val = pick::indToVec(i);
    pickColors.push_back(pick::indToVec(i));
  }


  // Store data in buffers
  pickProgram->setAttribute("a_position", points);
  pickProgram->setAttribute("a_color", pickColors);
}

void PointCloud::setUseBillboardSpheres(bool newValue) {
  useBillboardSpheres = newValue;
  deleteProgram();
}

bool PointCloud::requestsBillboardSpheres() const { return useBillboardSpheres; }

void PointCloud::drawSharedStructureUI() {}

void PointCloud::drawPickUI(size_t localPickID) {

  ImGui::TextUnformatted(("#" + std::to_string(localPickID) + "  ").c_str());
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << points[localPickID];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildInfoGUI(localPickID);
  }

  ImGui::Indent(-20.);
}

void PointCloud::drawUI() {

  ImGui::PushID(name.c_str()); // ensure there are no conflicts with identically-named labels

  if (ImGui::TreeNode(name.c_str())) {

    // Print stats
    ImGui::Text("# points: %lld", static_cast<long long int>(points.size()));


    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Point color", (float*)&pointColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();

    // Options popup
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      // Quantities
      if (ImGui::MenuItem("Use billboard spheres", NULL, &useBillboardSpheres))
        setUseBillboardSpheres(useBillboardSpheres);
      if (ImGui::MenuItem("Clear Quantities")) removeAllQuantities();
      if (ImGui::MenuItem("Write points to file")) writePointsToFile();


      ImGui::EndPopup();
    }

    ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);

    // Build the quantity UIs
    for (auto x : quantities) {
      x.second->drawUI();
    }

    ImGui::TreePop();
  }
  ImGui::PopID();
}

double PointCloud::lengthScale() {

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (Vector3& p : points) {
    lengthScale = std::max(lengthScale, geometrycentral::norm2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> PointCloud::boundingBox() {

  Vector3 min = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (Vector3& p : points) {
    min = geometrycentral::componentwiseMin(min, p);
    max = geometrycentral::componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

// === Quantities

PointCloudQuantity::PointCloudQuantity(std::string name_, PointCloud* pointCloud_) : name(name_), parent(pointCloud_) {}
PointCloudQuantityThatDrawsPoints::PointCloudQuantityThatDrawsPoints(std::string name_, PointCloud* pointCloud_)
    : PointCloudQuantity(name_, pointCloud_) {}
PointCloudQuantity::~PointCloudQuantity() {}

void PointCloud::addQuantity(PointCloudQuantity* quantity) {

  // Delete old if in use
  bool wasEnabled = false;
  if (quantities.find(quantity->name) != quantities.end()) {
    wasEnabled = quantities[quantity->name]->enabled;
    removeQuantity(quantity->name);
  }

  // Store
  quantities[quantity->name] = quantity;

  // Re-enable the quantity if we're replacing an enabled quantity
  if (wasEnabled) {
    quantity->enabled = true;
  }
}

void PointCloud::addQuantity(PointCloudQuantityThatDrawsPoints* quantity) {

  // Delete old if in use
  bool wasEnabled = false;
  if (quantities.find(quantity->name) != quantities.end()) {
    wasEnabled = quantities[quantity->name]->enabled;
    removeQuantity(quantity->name);
  }

  // Store
  quantities[quantity->name] = quantity;

  // Re-enable the quantity if we're replacing an enabled quantity
  if (wasEnabled) {
    quantity->enabled = true;
    setActiveQuantity(quantity);
  }
}

PointCloudQuantity* PointCloud::getQuantity(std::string name, bool errorIfAbsent) {

  // Check if exists
  if (quantities.find(name) == quantities.end()) {
    if (errorIfAbsent) {
      polyscope::error("No quantity named " + name + " registered");
    }
    return nullptr;
  }

  return quantities[name];
}

void PointCloud::addScalarQuantity(std::string name, const std::vector<double>& value, DataType type) {
  PointCloudQuantityThatDrawsPoints* q = new PointCloudScalarQuantity(name, value, this, type);
  addQuantity(q);
}

void PointCloud::addColorQuantity(std::string name, const std::vector<Vector3>& value) {
  PointCloudQuantityThatDrawsPoints* q = new PointCloudColorQuantity(name, value, this);
  addQuantity(q);
}

void PointCloud::addVectorQuantity(std::string name, const std::vector<Vector3>& vectors, VectorType vectorType) {
  PointCloudQuantity* q = new PointCloudVectorQuantity(name, vectors, this, vectorType);
  addQuantity(q);
}

void PointCloud::removeQuantity(std::string name) {

  if (quantities.find(name) == quantities.end()) {
    return;
  }

  PointCloudQuantity* q = quantities[name];
  quantities.erase(name);
  if (activePointQuantity == q) {
    clearActiveQuantity();
  }
  delete q;
}

void PointCloud::setActiveQuantity(PointCloudQuantityThatDrawsPoints* q) {
  clearActiveQuantity();
  activePointQuantity = q;
  q->enabled = true;
}

void PointCloud::clearActiveQuantity() {
  deleteProgram();
  if (activePointQuantity != nullptr) {
    activePointQuantity->enabled = false;
    activePointQuantity = nullptr;
  }
}

void PointCloud::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
}

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
  outFile << "#displayradius " << (pointRadius * state::lengthScale) << endl;

  for (size_t i = 0; i < points.size(); i++) {
    outFile << points[i] << endl;
  }

  outFile.close();
}

void PointCloudQuantity::buildInfoGUI(size_t pointInd) {}
void PointCloudQuantity::draw() {}
void PointCloudQuantityThatDrawsPoints::setProgramValues(gl::GLProgram* program) {}
bool PointCloudQuantityThatDrawsPoints::wantsBillboardUniforms() { return true; }


} // namespace polyscope

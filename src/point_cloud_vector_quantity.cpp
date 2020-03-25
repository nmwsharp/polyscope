// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"

#include <fstream>
#include <iostream>


using std::cout;
using std::endl;

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                   PointCloud& pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_), vectorType(vectorType_), vectors(vectors_),
      vectorLengthMult(uniquePrefix() + "#vectorLengthMult",
                       vectorType == VectorType::AMBIENT ? absoluteValue(1.0) : relativeValue(0.02)),
      vectorRadius(uniquePrefix() + "#vectorRadius", relativeValue(0.0025)),
      vectorColor(uniquePrefix() + "#vectorColor", getNextUniqueColor()),
      material(uniquePrefix() + "#material", "clay")

{

  if (vectors.size() != parent.points.size()) {
    polyscope::error("Point cloud vector quantity " + name + " does not have same number of values (" +
                     std::to_string(vectors.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<glm::vec3>(vectors, DataType::MAGNITUDE);
  }
}

void PointCloudVectorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);

  program->setUniform("u_radius", vectorRadius.get().asAbsolute());
  program->setUniform("u_baseColor", vectorColor.get());

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", vectorLengthMult.get().asAbsolute());
  }
	
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  program->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
 	program->setUniform("u_viewport", render::engine->getCurrentViewport());

  program->draw();
}

void PointCloudVectorQuantity::createProgram() {
  program = render::engine->generateShaderProgram(
      {render::PASSTHRU_VECTOR_VERT_SHADER, render::VECTOR_GEOM_SHADER, render::VECTOR_FRAG_SHADER}, DrawMode::Points);

  // Fill buffers
  std::vector<glm::vec3> mappedVectors;
  for (glm::vec3 v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", parent.points);

  render::engine->setMaterial(*program, material.get());
}

void PointCloudVectorQuantity::geometryChanged() { program.reset(); }

void PointCloudVectorQuantity::buildCustomUI() {
  ImGui::SameLine();

  if (ImGui::ColorEdit3("Color", &vectorColor.get()[0], ImGuiColorEditFlags_NoInputs)) setVectorColor(getVectorColor());
  ImGui::SameLine();

  // === Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    if (ImGui::MenuItem("Write to file")) writeToFile();
    ImGui::EndPopup();
  }

  // Only get to set length for non-ambient vectors
  if (vectorType != VectorType::AMBIENT) {
    if (ImGui::SliderFloat("Length", vectorLengthMult.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
      vectorLengthMult.manuallyChanged();
      requestRedraw();
    }
  }

  if (ImGui::SliderFloat("Radius", vectorRadius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    vectorRadius.manuallyChanged();
    requestRedraw();
  }

  { // Draw max and min magnitude
    ImGui::TextUnformatted(mapper.printBounds().c_str());
  }
}

void PointCloudVectorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectors[ind];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectors[ind]));
  ImGui::NextColumn();
}

void PointCloudVectorQuantity::writeToFile(std::string filename) {

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  cout << "Writing surface vector quantity " << name << " to file " << filename << endl;

  std::ofstream outFile(filename);
  outFile << "#Vectors written by polyscope from Point Cloud Vector Quantity " << name << endl;
  outFile << "#displayradius " << vectorRadius.get().asAbsolute() << endl;
  outFile << "#displaylength " << vectorLengthMult.get().asAbsolute() << endl;

  for (size_t i = 0; i < vectors.size(); i++) {
    if (glm::length2(vectors[i]) > 0) {
      outFile << parent.points[i] << " " << vectors[i] << endl;
    }
  }

  outFile.close();
}

PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorLengthScale(double newLength, bool isRelative) {
  vectorLengthMult = ScaledValue<double>(newLength, isRelative);
  requestRedraw();
  return this;
}
double PointCloudVectorQuantity::getVectorLengthScale() { return vectorLengthMult.get().asAbsolute(); }
PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorRadius(double val, bool isRelative) {
  vectorRadius = ScaledValue<double>(val, isRelative);
  requestRedraw();
  return this;
}
double PointCloudVectorQuantity::getVectorRadius() { return vectorRadius.get().asAbsolute(); }
PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorColor(glm::vec3 color) {
  vectorColor = color;
  requestRedraw();
  return this;
}
glm::vec3 PointCloudVectorQuantity::getVectorColor() { return vectorColor.get(); }

PointCloudVectorQuantity* PointCloudVectorQuantity::setMaterial(std::string m) {
  material = m;
  if (program) render::engine->setMaterial(*program, getMaterial());
  requestRedraw();
  return this;
}
std::string PointCloudVectorQuantity::getMaterial() { return material.get(); }

std::string PointCloudVectorQuantity::niceName() { return name + " (vector)"; }

} // namespace polyscope

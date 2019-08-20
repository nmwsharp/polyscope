// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <fstream>
#include <iostream>


using std::cout;
using std::endl;

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                   PointCloud& pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_), vectorType(vectorType_), vectors(vectors_) {

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

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .02;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .0005;
  vectorColor = getNextUniqueColor();
}

void PointCloudVectorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);

  program->setUniform("u_radius", radiusMult * state::lengthScale);
  program->setUniform("u_color", vectorColor);

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", lengthMult * state::lengthScale);
  }

  program->draw();
}

void PointCloudVectorQuantity::createProgram() {
  program.reset(new gl::GLProgram(&gl::PASSTHRU_VECTOR_VERT_SHADER, &gl::VECTOR_GEOM_SHADER,
                                  &gl::SHINY_VECTOR_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  std::vector<glm::vec3> mappedVectors;
  for (glm::vec3 v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", parent.points);

  setMaterialForProgram(*program, "wax");
}

void PointCloudVectorQuantity::geometryChanged() {
  program.reset();
}

void PointCloudVectorQuantity::buildCustomUI() {
  ImGui::SameLine();

  ImGui::ColorEdit3("Color", (float*)&vectorColor, ImGuiColorEditFlags_NoInputs);
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
    ImGui::SliderFloat("Length", &lengthMult, 0.0, .1, "%.5f", 3.);
  }

  ImGui::SliderFloat("Radius", &radiusMult, 0.0, .1, "%.5f", 3.);

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
  outFile << "#displayradius " << (radiusMult * state::lengthScale) << endl;
  outFile << "#displaylength " << (lengthMult * state::lengthScale) << endl;

  for (size_t i = 0; i < vectors.size(); i++) {
    if (glm::length2(vectors[i]) > 0) {
      outFile << parent.points[i] << " " << vectors[i] << endl;
    }
  }

  outFile.close();
}

std::string PointCloudVectorQuantity::niceName() { return name + " (vector)"; }

} // namespace polyscope

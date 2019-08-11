// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <complex>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

CurveNetworkVectorQuantity::CurveNetworkVectorQuantity(std::string name, CurveNetwork& network_, VectorType vectorType_)
    : CurveNetworkQuantity(name, network_), vectorType(vectorType_) {}

void CurveNetworkVectorQuantity::prepareVectorMapper() {

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<glm::vec3>(vectors, DataType::MAGNITUDE);
  }

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .05;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .001;
  vectorColor = getNextUniqueColor();
}

void CurveNetworkVectorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) prepareProgram();

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

void CurveNetworkVectorQuantity::prepareProgram() {

  program.reset(new gl::GLProgram(&gl::PASSTHRU_VECTOR_VERT_SHADER, &gl::VECTOR_GEOM_SHADER,
                                  &gl::SHINY_VECTOR_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  std::vector<glm::vec3> mappedVectors;
  for (glm::vec3& v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", vectorRoots);

  setMaterialForProgram(*program, "wax");
}

void CurveNetworkVectorQuantity::buildCustomUI() {
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

  drawSubUI();
}

void CurveNetworkVectorQuantity::drawSubUI() {}

void CurveNetworkVectorQuantity::writeToFile(std::string filename) {

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  if (options::verbosity > 0) {
    cout << "Writing curve network vector quantity " << name << " to file " << filename << endl;
  }

  std::ofstream outFile(filename);
  outFile << "#Vectors written by polyscope from Curve Network Vector Quantity " << name << endl;
  outFile << "#displayradius " << (radiusMult * state::lengthScale) << endl;
  outFile << "#displaylength " << (lengthMult * state::lengthScale) << endl;

  for (size_t i = 0; i < vectors.size(); i++) {
    if (glm::length(vectors[i]) > 0) {
      outFile << vectorRoots[i] << " " << vectors[i] << endl;
    }
  }

  outFile.close();
}

// ========================================================
// ==========           Node Vector            ==========
// ========================================================

CurveNetworkNodeVectorQuantity::CurveNetworkNodeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)

    : CurveNetworkVectorQuantity(name, network_, vectorType_), vectorField(vectors_) {

  size_t i = 0;
  vectorRoots = parent.nodes;
  vectors = vectorField;

  prepareVectorMapper();
}

void CurveNetworkNodeVectorQuantity::buildNodeInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[iV];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iV]));
  ImGui::NextColumn();
}

std::string CurveNetworkNodeVectorQuantity::niceName() { return name + " (node vector)"; }

void CurveNetworkNodeVectorQuantity::geometryChanged() {
  vectorRoots = parent.nodes;
  program.reset();
}

// ========================================================
// ==========            Edge Vector             ==========
// ========================================================

CurveNetworkEdgeVectorQuantity::CurveNetworkEdgeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)
    : CurveNetworkVectorQuantity(name, network_, vectorType_), vectorField(vectors_) {

  // Copy the vectors
  vectors = vectorField;
  vectorRoots.resize(parent.nEdges());

  for (size_t iE = 0; iE < parent.nEdges(); iE++) {
    auto& edge = parent.edges[iE];
    size_t eTail = std::get<0>(edge);
    size_t eTip = std::get<1>(edge);

    vectorRoots[iE] = 0.5f * (parent.nodes[eTail] + parent.nodes[eTip]);
  }

  prepareVectorMapper();
}

void CurveNetworkEdgeVectorQuantity::buildEdgeInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[iF];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iF]));
  ImGui::NextColumn();
}

std::string CurveNetworkEdgeVectorQuantity::niceName() { return name + " (edge vector)"; }

void CurveNetworkEdgeVectorQuantity::geometryChanged() {
  vectorRoots.resize(parent.nEdges());
  for (size_t iE = 0; iE < parent.nEdges(); iE++) {
    auto& edge = parent.edges[iE];
    size_t eTail = std::get<0>(edge);
    size_t eTip = std::get<1>(edge);

    vectorRoots[iE] = 0.5f * (parent.nodes[eTail] + parent.nodes[eTip]);
  }

  program.reset();
}

} // namespace polyscope

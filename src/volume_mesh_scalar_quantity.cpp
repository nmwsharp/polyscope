// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/volume_mesh_scalar_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

VolumeMeshScalarQuantity::VolumeMeshScalarQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn_,
                                                   const std::vector<double>& values_, DataType dataType_)
    : VolumeMeshQuantity(name, mesh_, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

void VolumeMeshScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setVolumeMeshUniforms(*program);
  setScalarUniforms(*program);

  program->draw();
}


void VolumeMeshScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
}

void VolumeMeshScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string VolumeMeshScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

VolumeMeshVertexScalarQuantity::VolumeMeshVertexScalarQuantity(std::string name, const std::vector<double>& values_,
                                                               VolumeMesh& mesh_, DataType dataType_)
    : VolumeMeshScalarQuantity(name, mesh_, "vertex", values_, dataType_)

{
  hist.buildHistogram(values, parent.vertexAreas); // rebuild to incorporate weights
}

void VolumeMeshVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}


void VolumeMeshVertexScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<double> colorval;
  colorval.resize(3 * parent.nFacesTriangulation());

  size_t iF = 0;
  size_t iFront = 0;
  size_t iBack = 3 * parent.nFacesTriangulation() - 3;
  for (size_t iC = 0; iC < parent.nCells(); iC++) {
    const std::array<int64_t, 8>& cell = parent.cells[iC];
    VolumeCellType cellT = parent.cellType(iC);
    for (const std::vector<std::array<size_t, 3>>& face : parent.cellStencil(cellT)) {

      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];

        // (see note in VolumeMesh.cpp about sorting the buffer outside-first)
        size_t iData;
        if (parent.faceIsInterior[iF]) {
          iData = iBack;
          iBack -= 3;
        } else {
          iData = iFront;
          iFront += 3;
        }

        for (int k = 0; k < 3; k++) {
          colorval[iData + k] = values[cell[tri[k]]];
        }
      }

      iF++;
    }
  }

  // Store data in buffers
  p.setAttribute("a_value", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void VolumeMeshVertexScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[vInd]);
  ImGui::NextColumn();
}

// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

VolumeMeshCellScalarQuantity::VolumeMeshCellScalarQuantity(std::string name, const std::vector<double>& values_,
                                                           VolumeMesh& mesh_, DataType dataType_)
    : VolumeMeshScalarQuantity(name, mesh_, "cell", values_, dataType_)

{
  hist.buildHistogram(values, parent.faceAreas); // rebuild to incorporate weights
}

void VolumeMeshCellScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void VolumeMeshCellScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<double> colorval;
  colorval.resize(3 * parent.nFacesTriangulation());

  size_t iF = 0;
  size_t iFront = 0;
  size_t iBack = 3 * parent.nFacesTriangulation() - 3;
  for (size_t iC = 0; iC < parent.nCells(); iC++) {
    const std::array<int64_t, 8>& cell = parent.cells[iC];
    VolumeCellType cellT = parent.cellType(iC);
    for (const std::vector<std::array<size_t, 3>>& face : parent.cellStencil(cellT)) {

      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];

        // (see note in VolumeMesh.cpp about sorting the buffer outside-first)
        size_t iData;
        if (parent.faceIsInterior[iF]) {
          iData = iBack;
          iBack -= 3;
        } else {
          iData = iFront;
          iFront += 3;
        }

        for (int k = 0; k < 3; k++) {
          colorval[iData + k] = values[iC];
        }
      }

      iF++;
    }
  }

  // Store data in buffers
  p.setAttribute("a_value", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void VolumeMeshCellScalarQuantity::buildCellInfoGUI(size_t cInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[cInd]);
  ImGui::NextColumn();
}


} // namespace polyscope

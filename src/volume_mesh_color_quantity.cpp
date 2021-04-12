// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/volume_mesh_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

VolumeMeshColorQuantity::VolumeMeshColorQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn_)
    : VolumeMeshQuantity(name, mesh_, true), definedOn(definedOn_) {}

void VolumeMeshColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setVolumeMeshUniforms(*program);

  program->draw();
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

VolumeMeshVertexColorQuantity::VolumeMeshVertexColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                             VolumeMesh& mesh_)
    : VolumeMeshColorQuantity(name, mesh_, "vertex"), values(std::move(values_))

{}

void VolumeMeshVertexColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void VolumeMeshVertexColorQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
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
  p.setAttribute("a_color", colorval);
}

void VolumeMeshVertexColorQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[vInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string VolumeMeshColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void VolumeMeshColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ========================================================
// ==========            Cell Color              ==========
// ========================================================

VolumeMeshCellColorQuantity::VolumeMeshCellColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                         VolumeMesh& mesh_)
    : VolumeMeshColorQuantity(name, mesh_, "cell"), values(std::move(values_))

{}

void VolumeMeshCellColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void VolumeMeshCellColorQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
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
  p.setAttribute("a_color", colorval);
}

void VolumeMeshCellColorQuantity::buildCellInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[fInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[fInd];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope

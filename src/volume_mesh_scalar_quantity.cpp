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
  sliceProgram.reset();
  Quantity::refresh();
}

std::string VolumeMeshScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

VolumeMeshVertexScalarQuantity::VolumeMeshVertexScalarQuantity(std::string name, const std::vector<double>& values_,
                                                               VolumeMesh& mesh_, DataType dataType_)
    : VolumeMeshScalarQuantity(name, mesh_, "vertex", values_, dataType_), levelSetValue(0), isDrawingLevelSet(false),
      showQuantity(this)

{
  hist.buildHistogram(values, parent.vertexAreas); // rebuild to incorporate weights
  parent.refreshVolumeMeshListeners();             // just in case this quantity is being drawn
}
void VolumeMeshVertexScalarQuantity::fillLevelSetData(render::ShaderProgram& p) {
  std::vector<glm::vec3> point1;
  std::vector<glm::vec3> point2;
  std::vector<glm::vec3> point3;
  std::vector<glm::vec3> point4;
  std::vector<glm::vec3> slice1;
  std::vector<glm::vec3> slice2;
  std::vector<glm::vec3> slice3;
  std::vector<glm::vec3> slice4;
  size_t tetCount = parent.nTets();
  slice1.resize(tetCount);
  slice2.resize(tetCount);
  slice3.resize(tetCount);
  slice4.resize(tetCount);
  point1.resize(tetCount);
  point2.resize(tetCount);
  point3.resize(tetCount);
  point4.resize(tetCount);
  auto vertices = parent.vertices;
  for (size_t i = 0; i < parent.nTets(); i++) {
    point1[i] = vertices[parent.tets[i][0]];
    point2[i] = vertices[parent.tets[i][1]];
    point3[i] = vertices[parent.tets[i][2]];
    point4[i] = vertices[parent.tets[i][3]];
    slice1[i] = glm::vec3(values[parent.tets[i][0]], 0, 0);
    slice2[i] = glm::vec3(values[parent.tets[i][1]], 0, 0);
    slice3[i] = glm::vec3(values[parent.tets[i][2]], 0, 0);
    slice4[i] = glm::vec3(values[parent.tets[i][3]], 0, 0);
  }
  p.setAttribute("a_point_1", point1);
  p.setAttribute("a_point_2", point2);
  p.setAttribute("a_point_3", point3);
  p.setAttribute("a_point_4", point4);
  p.setAttribute("a_slice_1", slice1);
  p.setAttribute("a_slice_2", slice2);
  p.setAttribute("a_slice_3", slice3);
  p.setAttribute("a_slice_4", slice4);
}

void VolumeMeshVertexScalarQuantity::setLevelSetUniforms(render::ShaderProgram& p) {
  p.setUniform("u_sliceVector", glm::vec3(1, 0, 0));
  p.setUniform("u_slicePoint", levelSetValue);
}

void VolumeMeshVertexScalarQuantity::draw() {
  if (!isEnabled()) return;

  auto programToDraw = program;
  if (isDrawingLevelSet) {
    if (levelSetProgram == nullptr) {
      levelSetProgram = createSliceProgram();
      fillLevelSetData(*levelSetProgram);
    }
    setLevelSetUniforms(*levelSetProgram);
    programToDraw = levelSetProgram;
  } else if (program == nullptr) {
    createProgram();
    programToDraw = program;
  }

  // Set uniforms
  parent.setStructureUniforms(*programToDraw);
  parent.setVolumeMeshUniforms(*programToDraw);
  setScalarUniforms(*programToDraw);

  programToDraw->draw();
}

void VolumeMeshVertexScalarQuantity::setLevelSetValue(float f) { levelSetValue = f; }

void VolumeMeshVertexScalarQuantity::setEnabledLevelSet(bool v) {
  if (v) {
    isDrawingLevelSet = true;
    setEnabled(true);
    parent.setLevelSetQuantity(this);
  } else {
    isDrawingLevelSet = false;
    parent.setLevelSetQuantity(nullptr);
  }
}

void VolumeMeshVertexScalarQuantity::drawSlice(polyscope::SlicePlane* sp) {
  if (!isEnabled()) return;

  if (sliceProgram == nullptr) {
    sliceProgram = createSliceProgram();
  }
  parent.setStructureUniforms(*sliceProgram);
  // Ignore current slice plane
  sp->setSceneObjectUniforms(*sliceProgram, true);
  sp->setSliceGeomUniforms(*sliceProgram);
  parent.setVolumeMeshUniforms(*sliceProgram);
  setScalarUniforms(*sliceProgram);
  sliceProgram->draw();
}

void VolumeMeshVertexScalarQuantity::setLevelSetVisibleQuantity(std::string name) {
  auto pair = parent.quantities.find(name);
  if (pair == parent.quantities.end()) {
    return;
  }
  VolumeMeshQuantity* vmq = pair->second.get();
  VolumeMeshVertexScalarQuantity* q = dynamic_cast<VolumeMeshVertexScalarQuantity*>(vmq);
  if (q == nullptr) {
    return;
  }
  levelSetProgram = render::engine->requestShader(
      "SLICE_TETS", parent.addVolumeMeshRules(addScalarRules({"SLICE_TETS_PROPAGATE_VALUE"}), true, true));

  // Fill color buffers
  parent.fillSliceGeometryBuffers(*levelSetProgram);
  q->fillSliceColorBuffers(*levelSetProgram);
  render::engine->setMaterial(*levelSetProgram, parent.getMaterial());
  fillLevelSetData(*levelSetProgram);
  setLevelSetUniforms(*levelSetProgram);
  showQuantity = q;
}

void VolumeMeshVertexScalarQuantity::buildCustomUI() {
  VolumeMeshScalarQuantity::buildCustomUI();
  /* TODO disabled for now
  if (ImGui::Checkbox("Level Set", &isDrawingLevelSet)) {
    setEnabledLevelSet(isDrawingLevelSet);
  }
  if (isDrawingLevelSet) {
    ImGui::DragFloat("", &levelSetValue, 0.01f, (float)hist.colormapRange.first, (float)hist.colormapRange.second);
    if (ImGui::BeginMenu("Show Quantity")) {
      std::map<std::string, std::unique_ptr<polyscope::VolumeMeshQuantity>>::iterator it;
      for (it = parent.quantities.begin(); it != parent.quantities.end(); it++) {
        std::string quantityName = it->first;
        VolumeMeshQuantity* vmq = it->second.get();
        VolumeMeshVertexScalarQuantity* vmvsq = dynamic_cast<VolumeMeshVertexScalarQuantity*>(vmq);
        if (vmvsq != nullptr && ImGui::MenuItem(quantityName.c_str(), NULL, showQuantity == it->second.get())) {
          setLevelSetVisibleQuantity(quantityName);
        }
      }
      ImGui::EndMenu();
    }
  }
  */
}

void VolumeMeshVertexScalarQuantity::refresh() {
  VolumeMeshScalarQuantity::refresh();
  levelSetProgram.reset();
}

void VolumeMeshVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

std::shared_ptr<render::ShaderProgram> VolumeMeshVertexScalarQuantity::createSliceProgram() {
  std::shared_ptr<render::ShaderProgram> p = render::engine->requestShader(
      "SLICE_TETS", parent.addVolumeMeshRules(addScalarRules({"SLICE_TETS_PROPAGATE_VALUE"}), true, true));

  // Fill color buffers
  parent.fillSliceGeometryBuffers(*p);
  fillSliceColorBuffers(*p);
  render::engine->setMaterial(*p, parent.getMaterial());
  return p;
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

void VolumeMeshVertexScalarQuantity::fillSliceColorBuffers(render::ShaderProgram& p) {
  size_t tetCount = parent.nTets();
  std::vector<double> colorval_1;
  std::vector<double> colorval_2;
  std::vector<double> colorval_3;
  std::vector<double> colorval_4;

  colorval_1.resize(tetCount);
  colorval_2.resize(tetCount);
  colorval_3.resize(tetCount);
  colorval_4.resize(tetCount);

  auto vertices = parent.vertices;
  for (size_t iT = 0; iT < parent.tets.size(); iT++) {
    colorval_1[iT] = values[parent.tets[iT][0]];
    colorval_2[iT] = values[parent.tets[iT][1]];
    colorval_3[iT] = values[parent.tets[iT][2]];
    colorval_4[iT] = values[parent.tets[iT][3]];
  }

  // Store data in buffers
  p.setAttribute("a_value_1", colorval_1);
  p.setAttribute("a_value_2", colorval_2);
  p.setAttribute("a_value_3", colorval_3);
  p.setAttribute("a_value_4", colorval_4);
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

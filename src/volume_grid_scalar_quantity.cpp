// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_grid_scalar_quantity.h"

#define MC_CPP_USE_DOUBLE_PRECISION
#include "MarchingCube/MC.h"

namespace polyscope {

// ========================================================
// ==========            Node Scalar             ==========
// ========================================================

VolumeGridNodeScalarQuantity::VolumeGridNodeScalarQuantity(std::string name, VolumeGrid& grid_,
                                                           const std::vector<double>& values_, DataType dataType_)
    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_),
      gridcubeVizEnabled(parent.uniquePrefix() + "#" + name + "#gridcubeVizEnabled", true),
      isosurfaceVizEnabled(parent.uniquePrefix() + "#" + name + "#isosurfaceVizEnabled", false),
      isosurfaceLevel(parent.uniquePrefix() + "#" + name + "#isosurfaceLevel",
                      0.5 * (vizRange.second + vizRange.first)),
      isosurfaceColor(uniquePrefix() + "#" + name + "#isosurfaceColor", getNextUniqueColor()) {

  values.setTextureSize(parent.gridNodeDim.x, parent.gridNodeDim.y, parent.gridNodeDim.z);
}


void VolumeGridNodeScalarQuantity::buildCustomUI() {

  // Select which viz to use
  ImGui::SameLine();
  if (ImGui::Button("Mode")) {
    ImGui::OpenPopup("ModePopup");
  }
  if (ImGui::BeginPopup("ModePopup")) {
    // show toggles for each
    // ImGui::Indent(20);
    if (ImGui::MenuItem("Gridcube", NULL, &gridcubeVizEnabled.get())) setGridcubeVizEnabled(getGridcubeVizEnabled());
    if (ImGui::MenuItem("Isosurface", NULL, &isosurfaceVizEnabled.get()))
      setIsosurfaceVizEnabled(getIsosurfaceVizEnabled());
    // ImGui::Indent(-20);
    ImGui::EndPopup();
  }


  // == Options popup
  ImGui::SameLine();
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    buildScalarOptionsUI();
    ImGui::EndPopup();
  }

  if (gridcubeVizEnabled.get()) {
    buildScalarUI();
  }

  if (isosurfaceVizEnabled.get()) {
    ImGui::TextUnformatted("Isosurface:");
    // Color picker
    if (ImGui::ColorEdit3("##Color", &isosurfaceColor.get()[0], ImGuiColorEditFlags_NoInputs)) {
      setIsosurfaceColor(getIsosurfaceColor());
    }
    ImGui::SameLine();

    // Set isovalue
    ImGui::PushItemWidth(120);
    if (ImGui::SliderFloat("##Radius", &isosurfaceLevel.get(), vizRange.first, vizRange.second, "%.4e")) {
      // Note: we intentionally do this rather than calling setIsosurfaceLevel(), because that function immediately
      // recomputes the level set mesh, which is too expensive during user interaction
      isosurfaceLevel.manuallyChanged();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
      refresh();
    }
  }
}

std::string VolumeGridNodeScalarQuantity::niceName() { return name + " (node scalar)"; }

void VolumeGridNodeScalarQuantity::refresh() {
  gridcubeProgram.reset();
  isosurfaceProgram.reset();
}

void VolumeGridNodeScalarQuantity::draw() {
  if (!isEnabled()) return;

  // Draw the point viz
  if (gridcubeVizEnabled.get()) {
    if (gridcubeProgram == nullptr) {
      createGridcubeProgram();
    }

    // Set program uniforms
    parent.setStructureUniforms(*gridcubeProgram);
    parent.setGridCubeUniforms(*gridcubeProgram);
    setScalarUniforms(*gridcubeProgram);
    render::engine->setMaterialUniforms(*gridcubeProgram, parent.getMaterial());

    // Draw the actual grid
    render::engine->setBackfaceCull(true);
    gridcubeProgram->draw();
  }

  // Draw the isosurface program
  if (isosurfaceVizEnabled.get()) {
    if (isosurfaceProgram == nullptr) {
      createIsosurfaceProgram();
    }
    parent.setStructureUniforms(*isosurfaceProgram);
    // setScalarUniforms(*isosurfaceProgram);
    render::engine->setMaterialUniforms(*isosurfaceProgram, parent.getMaterial());
    isosurfaceProgram->setUniform("u_baseColor", getIsosurfaceColor());
    isosurfaceProgram->draw();
  }
}

void VolumeGridNodeScalarQuantity::createGridcubeProgram() {


  // clang-format off
  gridcubeProgram = render::engine->requestShader( "GRIDCUBE_PLANE", 
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addGridCubeRules(
          addScalarRules(
            {"GRIDCUBE_PROPAGATE_NODE_VALUE"}
          ), 
        true)
      )
    );
  // clang-format on

  gridcubeProgram->setAttribute("a_referencePosition", parent.gridPlaneReferencePositions.getRenderAttributeBuffer());
  gridcubeProgram->setAttribute("a_referenceNormal", parent.gridPlaneReferenceNormals.getRenderAttributeBuffer());
  gridcubeProgram->setAttribute("a_axisInd", parent.gridPlaneAxisInds.getRenderAttributeBuffer());

  gridcubeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*gridcubeProgram, parent.getMaterial());

  gridcubeProgram->setTextureFromBuffer("t_value", values.getRenderTextureBuffer().get());
  values.getRenderTextureBuffer().get()->setFilterMode(FilterMode::Linear);
}

void VolumeGridNodeScalarQuantity::createIsosurfaceProgram() {

  values.ensureHostBufferPopulated();

  // Extract the isosurface from the level set of the scalar field
  MC::mcMesh mesh;
  MC::marching_cube(&values.data.front(), isosurfaceLevel.get(), parent.gridSpacing().x, parent.gridSpacing().y,
                    parent.gridSpacing().z, mesh);

  // Transform the result to be aligned with our volume's spatial layout
  glm::vec3 scale = parent.gridSpacing();
  for (auto& p : mesh.vertices) {
    p = p * scale + parent.boundMin;
  }


  // Create a render program to draw it
  // clang-format off
  isosurfaceProgram = render::engine->requestShader(
      "INDEXED_MESH",
      render::engine->addMaterialRules(parent.getMaterial(), parent.addStructureRules({"SHADE_BASECOLOR"})));
  // clang-format on

  // Populate the program buffers with the extracted mesh
  isosurfaceProgram->setAttribute("a_position", mesh.vertices);
  isosurfaceProgram->setAttribute("a_normal", mesh.normals);
  isosurfaceProgram->setIndex(mesh.indices);

  // Fill out some barycoords
  // TODO: extract barycoords from surface mesh shader to rule so we don't have to add a useless quantity
  isosurfaceProgram->setAttribute("a_barycoord", mesh.normals); // unused

  render::engine->setMaterial(*isosurfaceProgram, parent.getMaterial());
}

void VolumeGridNodeScalarQuantity::buildNodeInfoGUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(ind));
  ImGui::NextColumn();
}

// === Getters and setters

VolumeGridNodeScalarQuantity* VolumeGridNodeScalarQuantity::setGridcubeVizEnabled(bool val) {
  gridcubeVizEnabled = val;
  requestRedraw();
  return this;
}
bool VolumeGridNodeScalarQuantity::getGridcubeVizEnabled() { return gridcubeVizEnabled.get(); }

VolumeGridNodeScalarQuantity* VolumeGridNodeScalarQuantity::setIsosurfaceVizEnabled(bool val) {
  isosurfaceVizEnabled = val;
  requestRedraw();
  return this;
}
bool VolumeGridNodeScalarQuantity::getIsosurfaceVizEnabled() { return isosurfaceVizEnabled.get(); }

VolumeGridNodeScalarQuantity* VolumeGridNodeScalarQuantity::setIsosurfaceLevel(float val) {
  isosurfaceLevel = val;
  isosurfaceProgram.reset(); // delete the program so it gets recreated with the new value
  requestRedraw();
  return this;
}
float VolumeGridNodeScalarQuantity::getIsosurfaceLevel() { return isosurfaceLevel.get(); }

VolumeGridNodeScalarQuantity* VolumeGridNodeScalarQuantity::setIsosurfaceColor(glm::vec3 val) {
  isosurfaceColor = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeGridNodeScalarQuantity::getIsosurfaceColor() { return isosurfaceColor.get(); }


// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

VolumeGridCellScalarQuantity::VolumeGridCellScalarQuantity(std::string name, VolumeGrid& grid_,
                                                           const std::vector<double>& values_, DataType dataType_)
    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_),
      gridcubeVizEnabled(parent.uniquePrefix() + "#" + name + "#gridcubeVizEnabled", true) {

  values.setTextureSize(parent.gridCellDim.x, parent.gridCellDim.y, parent.gridCellDim.z);
}


void VolumeGridCellScalarQuantity::buildCustomUI() {

  // Select which viz to use
  ImGui::SameLine();
  if (ImGui::Button("Mode")) {
    ImGui::OpenPopup("ModePopup");
  }
  if (ImGui::BeginPopup("ModePopup")) {
    // show toggles for each
    // ImGui::Indent(20);
    if (ImGui::MenuItem("Gridcube", NULL, &gridcubeVizEnabled.get())) setGridcubeVizEnabled(getGridcubeVizEnabled());
    // ImGui::Indent(-20);
    ImGui::EndPopup();
  }


  // == Options popup
  ImGui::SameLine();
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    buildScalarOptionsUI();
    ImGui::EndPopup();
  }

  if (gridcubeVizEnabled.get()) {
    buildScalarUI();
  }
}

std::string VolumeGridCellScalarQuantity::niceName() { return name + " (cell scalar)"; }

void VolumeGridCellScalarQuantity::refresh() { gridcubeProgram.reset(); }

void VolumeGridCellScalarQuantity::draw() {
  if (!isEnabled()) return;

  // Draw the point viz
  if (gridcubeVizEnabled.get()) {
    if (gridcubeProgram == nullptr) {
      createGridcubeProgram();
    }

    // Set program uniforms
    parent.setStructureUniforms(*gridcubeProgram);
    parent.setGridCubeUniforms(*gridcubeProgram);
    setScalarUniforms(*gridcubeProgram);
    render::engine->setMaterialUniforms(*gridcubeProgram, parent.getMaterial());

    // Draw the actual grid
    render::engine->setBackfaceCull(true);
    gridcubeProgram->draw();
  }
}

void VolumeGridCellScalarQuantity::createGridcubeProgram() {


  // clang-format off
  gridcubeProgram = render::engine->requestShader( "GRIDCUBE_PLANE", 
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addGridCubeRules(
          addScalarRules(
            {"GRIDCUBE_PROPAGATE_CELL_VALUE"}
          ), 
        true)
      )
  );
  // clang-format on

  gridcubeProgram->setAttribute("a_referencePosition", parent.gridPlaneReferencePositions.getRenderAttributeBuffer());
  gridcubeProgram->setAttribute("a_referenceNormal", parent.gridPlaneReferenceNormals.getRenderAttributeBuffer());
  gridcubeProgram->setAttribute("a_axisInd", parent.gridPlaneAxisInds.getRenderAttributeBuffer());

  gridcubeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*gridcubeProgram, parent.getMaterial());

  gridcubeProgram->setTextureFromBuffer("t_value", values.getRenderTextureBuffer().get());
  values.getRenderTextureBuffer().get()->setFilterMode(FilterMode::Linear);
}

void VolumeGridCellScalarQuantity::buildCellInfoGUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(ind));
  ImGui::NextColumn();
}

// === Getters and setters

VolumeGridCellScalarQuantity* VolumeGridCellScalarQuantity::setGridcubeVizEnabled(bool val) {
  gridcubeVizEnabled = val;
  requestRedraw();
  return this;
}
bool VolumeGridCellScalarQuantity::getGridcubeVizEnabled() { return gridcubeVizEnabled.get(); }


} // namespace polyscope

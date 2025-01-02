// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_grid_scalar_quantity.h"

#include "MarchingCube/MC.h"

namespace polyscope {

// ========================================================
// ==========            Node Scalar             ==========
// ========================================================

VolumeGridNodeScalarQuantity::VolumeGridNodeScalarQuantity(std::string name, VolumeGrid& grid_,
                                                           const std::vector<float>& values_, DataType dataType_)
    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_),
      gridcubeVizEnabled(uniquePrefix() + "gridcubeVizEnabled", true),
      isosurfaceVizEnabled(uniquePrefix() + "isosurfaceVizEnabled", false),
      isosurfaceLevel(uniquePrefix() + "isosurfaceLevel", 0.f),
      isosurfaceColor(uniquePrefix() + "isosurfaceColor", getNextUniqueColor()),
      slicePlanesAffectIsosurface(uniquePrefix() + "slicePlanesAffectIsosurface", false) {

  values.setTextureSize(parent.getGridNodeDim().x, parent.getGridNodeDim().y, parent.getGridNodeDim().z);
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

    if (ImGui::MenuItem("Slice plane affects isosurface", NULL, &slicePlanesAffectIsosurface.get()))
      setSlicePlanesAffectIsosurface(getSlicePlanesAffectIsosurface());

    if (ImGui::MenuItem("Register isosurface as mesh")) registerIsosurfaceAsMesh();

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
    if (ImGui::SliderFloat("##Radius", &isosurfaceLevel.get(), vizRangeMin.get(), vizRangeMax.get(), "%.4e")) {
      // Note: we intentionally do this rather than calling setIsosurfaceLevel(), because that function immediately
      // recomputes the levelset mesh, which is too expensive during user interaction
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

bool VolumeGridNodeScalarQuantity::isDrawingGridcubes() { return isEnabled() && getGridcubeVizEnabled(); }

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

    glm::mat4 P = view::getCameraPerspectiveMatrix();
    glm::mat4 Pinv = glm::inverse(P);
    isosurfaceProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
    isosurfaceProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

    render::engine->setBackfaceCull(false);
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
  MC::mcMesh isosurfaceMesh;
  MC::marching_cube(&values.data.front(), isosurfaceLevel.get(), parent.getGridNodeDim().x, parent.getGridNodeDim().y,
                    parent.getGridNodeDim().z, isosurfaceMesh);

  // Transform the result to be aligned with our volume's spatial layout
  glm::vec3 scale = parent.gridSpacing();
  for (auto& p : isosurfaceMesh.vertices) {
    // swizzle to account for change of coordinate/buffer ordering in the MC lib
    p = glm::vec3{p.z, p.y, p.x} * scale + parent.getBoundMin();
  }

  std::vector<std::string> isoProgramRules{"SHADE_BASECOLOR", "PROJ_AND_INV_PROJ_MAT",
                                           "COMPUTE_SHADE_NORMAL_FROM_POSITION"};
  if (getSlicePlanesAffectIsosurface() && render::engine->slicePlanesEnabled()) {
    isoProgramRules.push_back("GENERATE_VIEW_POS");
    isoProgramRules.push_back("CULL_POS_FROM_VIEW");
  }

  // Create a render program to draw it
  // clang-format off
  isosurfaceProgram = render::engine->requestShader("SIMPLE_MESH",
      render::engine->addMaterialRules(parent.getMaterial(), 
        parent.addStructureRules(
          isoProgramRules
        )
      ),
    getSlicePlanesAffectIsosurface() ? 
     render::ShaderReplacementDefaults::SceneObject :
     render::ShaderReplacementDefaults::SceneObjectNoSlice
    );
  // clang-format on

  // Populate the program buffers with the extracted mesh
  isosurfaceProgram->setAttribute("a_vertexPositions", isosurfaceMesh.vertices);
  std::shared_ptr<render::AttributeBuffer> indexBuff = render::engine->generateAttributeBuffer(RenderDataType::UInt);
  indexBuff->setData(isosurfaceMesh.indices);
  isosurfaceProgram->setIndex(indexBuff);


  render::engine->setMaterial(*isosurfaceProgram, parent.getMaterial());
}

SurfaceMesh* VolumeGridNodeScalarQuantity::registerIsosurfaceAsMesh(std::string structureName) {

  // set the name to default
  if (structureName == "") {
    structureName = parent.name + " - " + name + " - isosurface";
  }

  // extract the mesh
  MC::mcMesh isosurfaceMesh;
  MC::marching_cube(&values.data.front(), isosurfaceLevel.get(), parent.getGridNodeDim().x, parent.getGridNodeDim().y,
                    parent.getGridNodeDim().z, isosurfaceMesh);
  glm::vec3 scale = parent.gridSpacing();
  for (auto& p : isosurfaceMesh.vertices) {
    // swizzle to account for change of coordinate/buffer ordering in the MC lib
    p = glm::vec3{p.z, p.y, p.x} * scale + parent.getBoundMin();
  }

  return registerSurfaceMesh(structureName, isosurfaceMesh.vertices,
                             std::make_tuple(isosurfaceMesh.indices.data(), isosurfaceMesh.indices.size() / 3, 3));
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

VolumeGridNodeScalarQuantity* VolumeGridNodeScalarQuantity::setSlicePlanesAffectIsosurface(bool val) {
  slicePlanesAffectIsosurface = val;
  isosurfaceProgram.reset(); // delete the program so it gets recreated with the new value
  requestRedraw();
  return this;
}
bool VolumeGridNodeScalarQuantity::getSlicePlanesAffectIsosurface() { return slicePlanesAffectIsosurface.get(); }

// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

VolumeGridCellScalarQuantity::VolumeGridCellScalarQuantity(std::string name, VolumeGrid& grid_,
                                                           const std::vector<float>& values_, DataType dataType_)
    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_),
      gridcubeVizEnabled(parent.uniquePrefix() + "#" + name + "#gridcubeVizEnabled", true) {

  values.setTextureSize(parent.getGridCellDim().x, parent.getGridCellDim().y, parent.getGridCellDim().z);
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

bool VolumeGridCellScalarQuantity::isDrawingGridcubes() { return isEnabled() && getGridcubeVizEnabled(); }

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
  gridcubeProgram = render::engine->requestShader("GRIDCUBE_PLANE", 
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

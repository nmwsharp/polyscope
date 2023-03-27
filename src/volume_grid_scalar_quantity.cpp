// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_grid_scalar_quantity.h"

#define MC_CPP_USE_DOUBLE_PRECISION
#include "MarchingCube/MC.h"

namespace polyscope {

VolumeGridScalarQuantity::VolumeGridScalarQuantity(std::string name, VolumeGrid& grid_,
                                                   const std::vector<double>& values_, DataType dataType_)

    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_), dataType(dataType_),
      values(std::move(values_)), pointVizEnabled(parent.uniquePrefix() + "#" + name + "#pointVizEnabled", true),
      isosurfaceVizEnabled(parent.uniquePrefix() + "#" + name + "#isosurfaceVizEnabled", true),
      isosurfaceLevel(parent.uniquePrefix() + "#" + name + "#isosurfaceLevel",
                      0.5 * (vizRange.second + vizRange.first)),
      isosurfaceColor(uniquePrefix() + "#" + name + "#isosurfaceColor", getNextUniqueColor())

{
  fillPositions();
}
void VolumeGridScalarQuantity::buildCustomUI() {

  // Select which viz to use
  ImGui::SameLine();
  if (ImGui::Button("Mode")) {
    ImGui::OpenPopup("ModePopup");
  }
  if (ImGui::BeginPopup("ModePopup")) {
    // show toggles for each
    // ImGui::Indent(20);
    if (ImGui::MenuItem("Points", NULL, &pointVizEnabled.get())) setPointVizEnabled(getPointVizEnabled());
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

  if (pointVizEnabled.get()) {
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

std::string VolumeGridScalarQuantity::niceName() { return name + " (scalar)"; }

void VolumeGridScalarQuantity::refresh() {
  pointProgram.reset();
  isosurfaceProgram.reset();
}

void VolumeGridScalarQuantity::fillPositions() {
  positions.clear();
  // Fill the positions vector with each grid corner position
  size_t nValues = parent.nValues();
  positions.resize(nValues);
  for (size_t i = 0; i < nValues; i++) {
    positions[i] = parent.positionOfIndex(i);
  }
}

void VolumeGridScalarQuantity::draw() {
  if (!isEnabled()) return;

  // Draw the point viz
  if (pointVizEnabled.get()) {
    if (pointProgram == nullptr) {
      createPointProgram();
    }
    parent.setStructureUniforms(*pointProgram);
    parent.setVolumeGridUniforms(*pointProgram);
    parent.setVolumeGridPointUniforms(*pointProgram);
    setScalarUniforms(*pointProgram);
    pointProgram->draw();
  }

  // Draw the isosurface program
  if (isosurfaceVizEnabled.get()) {
    if (isosurfaceProgram == nullptr) {
      createIsosurfaceProgram();
    }
    parent.setStructureUniforms(*isosurfaceProgram);
    // parent.setVolumeGridPointUniforms(*isosurfaceProgram);
    // setScalarUniforms(*isosurfaceProgram);
    isosurfaceProgram->setUniform("u_baseColor", getIsosurfaceColor());

    isosurfaceProgram->draw();
  }
}

void VolumeGridScalarQuantity::createPointProgram() {

  pointProgram = render::engine->requestShader(
      "RAYCAST_SPHERE", parent.addVolumeGridPointRules(addScalarRules({"SPHERE_PROPAGATE_VALUE"})));

  // Fill buffers
  pointProgram->setAttribute("a_position", parent.gridPointLocations);
  pointProgram->setAttribute("a_value", values);
  pointProgram->setTextureFromColormap("t_colormap", cMap.get());

  render::engine->setMaterial(*pointProgram, parent.getMaterial());
}

void VolumeGridScalarQuantity::createIsosurfaceProgram() {

  // Extract the isosurface from the level set of the scalar field
  MC::mcMesh mesh;
  MC::marching_cube(&values.front(), isosurfaceLevel.get(), parent.steps[0], parent.steps[1], parent.steps[2], mesh);

  // Transform the result to be aligned with our volume's spatial layout
  glm::vec3 scale{(parent.bound_max[0] - parent.bound_min[0]) / (parent.steps[0] - 1),
                  (parent.bound_max[1] - parent.bound_min[1]) / (parent.steps[1] - 1),
                  (parent.bound_max[2] - parent.bound_min[2]) / (parent.steps[2] - 1)};
  for (auto& p : mesh.vertices) {
    p = p * scale + parent.bound_min;
  }


  // Create a render program to draw it
  isosurfaceProgram = render::engine->requestShader("INDEXED_MESH", parent.addStructureRules({"SHADE_BASECOLOR"}));

  // Populate the program buffers with the extracted mesh
  isosurfaceProgram->setAttribute("a_position", mesh.vertices);
  isosurfaceProgram->setAttribute("a_normal", mesh.normals);
  isosurfaceProgram->setIndex(mesh.indices);

  // Fill out some barycoords
  // TODO: extract barycoords from surface mesh shader to rule so we don't have to add a useless quantity
  isosurfaceProgram->setAttribute("a_barycoord", mesh.normals); // unused

  render::engine->setMaterial(*isosurfaceProgram, parent.getMaterial());
}

// === Getters and setters

VolumeGridScalarQuantity* VolumeGridScalarQuantity::setPointVizEnabled(bool val) {
  pointVizEnabled = val;
  requestRedraw();
  return this;
}
bool VolumeGridScalarQuantity::getPointVizEnabled() { return pointVizEnabled.get(); }

VolumeGridScalarQuantity* VolumeGridScalarQuantity::setIsosurfaceVizEnabled(bool val) {
  isosurfaceVizEnabled = val;
  requestRedraw();
  return this;
}
bool VolumeGridScalarQuantity::getIsosurfaceVizEnabled() { return isosurfaceVizEnabled.get(); }

VolumeGridScalarQuantity* VolumeGridScalarQuantity::setIsosurfaceLevel(float val) {
  isosurfaceLevel = val;
  isosurfaceProgram.reset(); // delete the program so it gets recreated with the new value
  requestRedraw();
  return this;
}
float VolumeGridScalarQuantity::getIsosurfaceLevel() { return isosurfaceLevel.get(); }

VolumeGridScalarQuantity* VolumeGridScalarQuantity::setIsosurfaceColor(glm::vec3 val) {
  isosurfaceColor = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeGridScalarQuantity::getIsosurfaceColor() { return isosurfaceColor.get(); }

} // namespace polyscope

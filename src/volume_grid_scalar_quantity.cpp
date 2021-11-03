// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/volume_grid_scalar_quantity.h"


namespace polyscope {

VolumeGridScalarQuantity::VolumeGridScalarQuantity(std::string name, VolumeGrid& grid_,
                                                   const std::vector<double>& values_, DataType dataType_)

    : VolumeGridQuantity(name, grid_, true), ScalarQuantity(*this, values_, dataType_), dataType(dataType_),
      values(std::move(values_)), pointVizEnabled(parent.uniquePrefix() + "#" + name + "#pointVizEnabled", true) {

  if (values_.size() != parent.nValues()) {
    polyscope::error("Volume grid scalar quantity " + name + " does not have same number of values (" +
                     std::to_string(values_.size()) + ") as volume grid (" + std::to_string(parent.nValues()) + ")");
  }

  fillPositions();
}
void VolumeGridScalarQuantity::buildCustomUI() {}
std::string VolumeGridScalarQuantity::niceName() { return name + " (scalar)"; }

void VolumeGridScalarQuantity::refresh() { pointProgram.reset(); }

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
} // namespace polyscope

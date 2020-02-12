#include "polyscope/volumetric_grid_scalar_isosurface.h"

namespace polyscope {

VolumetricGridScalarIsosurface::VolumetricGridScalarIsosurface(std::string name, VolumetricGrid& grid_, const std::vector<double> &values_)
    : VolumetricGridQuantity(name, grid_, true), values(std::move(values_)) {}

void VolumetricGridScalarIsosurface::draw() {
  if (!isEnabled()) return;
  if (meshProgram == nullptr) {
    createProgram();
    return;
  }
  // Set uniforms
  parent.setTransformUniforms(*meshProgram);
  setProgramUniforms(*meshProgram);
  meshProgram->draw();
}

void VolumetricGridScalarIsosurface::buildCustomUI() {
  ImGui::Text("TODO: create the UI for this quantity");
}

void VolumetricGridScalarIsosurface::setProgramUniforms(gl::GLProgram& program) {
  // For now, nothing
}

std::string VolumetricGridScalarIsosurface::niceName() {
  return name + " (isosurface)"; 
}

void VolumetricGridScalarIsosurface::geometryChanged() {
  // For now, nothing
}

void VolumetricGridScalarIsosurface::createProgram() {
  // TODO: figure out what shaders to use
}

} // namespace polyscope

#include "polyscope/volumetric_grid_scalar_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"


namespace polyscope {

VolumetricGridScalarQuantity::VolumetricGridScalarQuantity(std::string name, VolumetricGrid& grid_,
                                                           const std::vector<double>& values_, DataType dataType_)
    : VolumetricGridQuantity(name, grid_, true), dataType(dataType_), values(std::move(values_)),
      cMap(uniquePrefix() + "#cmap", defaultColorMap(dataType)) {

  fillPositions();
  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
  pointRadius = 0.5 * parent.sideLength / (parent.nCornersPerSide - 1);
}


void VolumetricGridScalarQuantity::buildCustomUI() {
  ImGui::SliderFloat("Point radius", &pointRadius, 0, 1);
}

std::string VolumetricGridScalarQuantity::niceName() { return name + " (scalar)"; }

void VolumetricGridScalarQuantity::geometryChanged() {
  // For now, nothing
}

void VolumetricGridScalarQuantity::fillPositions() {
  positions.clear();
  // Fill the positions vector with each grid corner position
  size_t nValues = parent.nValues();
  positions.resize(nValues);
  for (size_t i = 0; i < nValues; i++) {
    positions[i] = parent.positionOfIndex(i);
  }
}

void VolumetricGridScalarQuantity::draw() {
  if (!isEnabled()) return;
  
  if (pointsProgram == nullptr) {
    createProgram();
  }
  parent.setTransformUniforms(*pointsProgram);
  setPointCloudUniforms();
  pointsProgram->setUniform("u_rangeLow", vizRange.first);
  pointsProgram->setUniform("u_rangeHigh", vizRange.second);

  pointsProgram->draw();
}

void VolumetricGridScalarQuantity::resetMapRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRange = dataRange;
    break;
  case DataType::SYMMETRIC: {
    double absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
    vizRange = std::make_pair(-absRange, absRange);
  } break;
  case DataType::MAGNITUDE:
    vizRange = std::make_pair(0., dataRange.second);
    break;
  }

  requestRedraw();
}

void VolumetricGridScalarQuantity::setPointCloudUniforms() {
  pointsProgram->setUniform("u_pointRadius", pointRadius);

  glm::vec3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);
  pointsProgram->setUniform("u_camZ", lookDir);
  pointsProgram->setUniform("u_camUp", upDir);
  pointsProgram->setUniform("u_camRight", rightDir);
}

void VolumetricGridScalarQuantity::createProgram() {
  pointsProgram.reset(new gl::GLProgram(&gl::SPHERE_VALUE_VERT_SHADER, &gl::SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                        &gl::SPHERE_VALUE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));
  gl::setMaterialForProgram(*pointsProgram, "wax");

  pointsProgram->setAttribute("a_position", positions);
  pointsProgram->setAttribute("a_value", values);
  pointsProgram->setTextureFromColormap("t_colormap", gl::getColorMap(cMap.get()));
}

} // namespace polyscope
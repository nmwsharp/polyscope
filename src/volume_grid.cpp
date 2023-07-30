// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/volume_grid.h"

#include "imgui.h"

namespace polyscope {

// Initialize statics
const std::string VolumeGrid::structureTypeName = "Volume Grid";

VolumeGrid::VolumeGrid(std::string name, glm::uvec3 gridNodeDim_, glm::vec3 bound_min_, glm::vec3 bound_max_)
    : QuantityStructure<VolumeGrid>(name, typeName()), gridNodeDim(gridNodeDim_), gridCellDim(gridNodeDim_ - 1u),
      bound_min(bound_min_), bound_max(bound_max_),

      // clang-format off
      // == managed quantities
      cellCenters(uniquePrefix() + "#cellCenters",  cellCentersData,    std::bind(&VolumeGrid::computeCellCenters, this)),
      cellInds(uniquePrefix() + "#cellInds",        cellIndsData,       std::bind(&VolumeGrid::computeCellInds, this)),

      // == persistent options
      color(                  uniquePrefix() + "color",         getNextUniqueColor()),
      edgeColor(              uniquePrefix() + "edgeColor",     glm::vec3{0., 0., 0.}), 
      material(               uniquePrefix() + "material",      "clay"),
      edgeWidth(              uniquePrefix() + "edgeWidth",     0.),
      cubeSizeFactor(         uniquePrefix() + "cubeSizeFactor",     .9)
// clang-format on
{
  updateObjectSpaceBounds();
}

void VolumeGrid::buildCustomUI() {
  ImGui::Text("samples: %lld  (%lld, %lld, %lld)", static_cast<long long int>(nNodes()),
              static_cast<long long int>(gridNodeDim.x), static_cast<long long int>(gridNodeDim.y),
              static_cast<long long int>(gridNodeDim.z));
  ImGui::TextUnformatted(("min: " + to_string_short(bound_min)).c_str());
  ImGui::TextUnformatted(("max: " + to_string_short(bound_max)).c_str());


  if (ImGui::SliderFloat("Cube Size", &cubeSizeFactor.get(), 0.001, 1.)) {
    cubeSizeFactor.manuallyChanged();
    requestRedraw();
  }
}

void VolumeGrid::buildPickUI(size_t localPickID) {
  // For now do nothing
}

void VolumeGrid::draw() {
  // For now, do nothing for the actual grid
  if (!enabled.get()) return;

  // If there is no dominant quantity, then this class is responsible for the grid
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    ensureGridCubeRenderProgramPrepared();

    // Set program uniforms
    setStructureUniforms(*program);
    setGridCubeUniforms(*program);
    program->setUniform("u_baseColor", color.get());

    // Draw the actual grid
    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void VolumeGrid::drawDelayed() {
  // For now, do nothing for the actual grid
  if (!enabled.get()) return;

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}

void VolumeGrid::drawPick() {
  // For now do nothing
}

std::vector<std::string> VolumeGrid::addGridCubeRules(std::vector<std::string> initRules) {
  initRules = addStructureRules(initRules);
  initRules.push_back("MESH_COMPUTE_NORMAL_FROM_POSITION");
  return initRules;
}

void VolumeGrid::setGridCubeUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());

  p.setUniform("u_gridSpacing", gridSpacing());
  p.setUniform("u_cubeSizeFactor", cubeSizeFactor.get());
}

void VolumeGrid::ensureGridCubeRenderProgramPrepared() {
  // If already prepared, do nothing
  if (program) return;

  // clang-format off
  program = render::engine->requestShader(
      "GRIDCUBE", 
      addGridCubeRules({"SHADE_BASECOLOR"})
  );
  // clang-format on

  program->setAttribute("a_cellPosition", cellCenters.getRenderAttributeBuffer());
  program->setAttribute("a_cellInd", cellInds.getRenderAttributeBuffer());

  render::engine->setMaterial(*program, material.get());
}

void VolumeGrid::ensureGridCubePickProgramPrepared() {
  // If already prepared, do nothing
  if (program) return;

  // TODO

  //   // clang-format off
  //   program = render::engine->requestShader(
  //       "GRIDCUBE",
  //       addGridCubeRules({""})
  //   );
  //   // clang-format on
  //
  //   setPointProgramGeometryAttributes(*program);
  //
  //   render::engine->setMaterial(*program, material.get());
}


void VolumeGrid::updateObjectSpaceBounds() {
  objectSpaceBoundingBox = std::make_tuple(bound_min, bound_max);
  objectSpaceLengthScale = glm::length(bound_max - bound_min);
}

std::string VolumeGrid::typeName() { return structureTypeName; }

void VolumeGrid::refresh() {
  QuantityStructure<VolumeGrid>::refresh(); // call base class version, which refreshes quantities
}


// void VolumeGrid::populateGeometry() {
//   gridPointLocations.resize(nNodes());
//   for (size_t i = 0; i < gridPointLocations.size(); i++) {
//     gridPointLocations[i] = positionOfNodeIndex(i);
//   }
// }


void VolumeGrid::setVolumeGridUniforms(render::ShaderProgram& p) {}

// void VolumeGrid::setVolumeGridPointUniforms(render::ShaderProgram& p) {
//   glm::mat4 P = view::getCameraPerspectiveMatrix();
//   glm::mat4 Pinv = glm::inverse(P);
//   p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
//   p.setUniform("u_viewport", render::engine->getCurrentViewport());
//   float pointRadius = minGridSpacing() / 8;
//   p.setUniform("u_pointRadius", pointRadius);
// }


// std::vector<std::string> VolumeGrid::addVolumeGridPointRules(std::vector<std::string> initRules) {
//   initRules = addStructureRules(initRules);
//   if (wantsCullPosition()) {
//     initRules.push_back("SPHERE_CULLPOS_FROM_CENTER");
//   }
//   return initRules;
// }

void VolumeGrid::computeCellInds() {

  cellInds.data.clear();
  cellInds.data.resize(nCells());

  glm::vec3 pad = gridSpacing();

  size_t iN = 0;
  for (size_t iX = 0; iX < gridCellDim.x; iX++) {
    for (size_t iY = 0; iY < gridCellDim.y; iY++) {
      for (size_t iZ = 0; iZ < gridCellDim.z; iZ++) {
        cellInds.data[iN] = glm::vec3{iX, iY, iZ};
        iN++;
      }
    }
  }

  cellInds.markHostBufferUpdated();
}

void VolumeGrid::computeCellCenters() {

  cellCenters.data.clear();
  cellCenters.data.resize(nCells());

  glm::vec3 pad = gridSpacing() / 2.f;
  glm::vec3 width = bound_max - bound_min;

  size_t iN = 0;
  for (size_t iX = 0; iX < gridCellDim.x; iX++) {
    float tX = static_cast<float>(iX) / gridCellDim.x;
    float pX = tX * width.x + bound_min.x + pad.x;

    for (size_t iY = 0; iY < gridCellDim.y; iY++) {
      float tY = static_cast<float>(iY) / gridCellDim.y;
      float pY = tY * width.y + bound_min.y + pad.y;

      for (size_t iZ = 0; iZ < gridCellDim.z; iZ++) {
        float tZ = static_cast<float>(iZ) / gridCellDim.z;
        float pZ = tZ * width.z + bound_min.z + pad.z;

        cellCenters.data[iN] = glm::vec3{pX, pY, pZ};
        iN++;
      }
    }
  }

  cellCenters.markHostBufferUpdated();
}

// === Option getters and setters

VolumeGrid* VolumeGrid::setColor(glm::vec3 val) {
  color = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeGrid::getColor() { return color.get(); }

VolumeGrid* VolumeGrid::setEdgeColor(glm::vec3 val) {
  edgeColor = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeGrid::getEdgeColor() { return edgeColor.get(); }

VolumeGrid* VolumeGrid::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string VolumeGrid::getMaterial() { return material.get(); }

VolumeGrid* VolumeGrid::setEdgeWidth(double newVal) {
  edgeWidth = newVal;
  refresh();
  requestRedraw();
  return this;
}
double VolumeGrid::getEdgeWidth() { return edgeWidth.get(); }


// === Register functions


VolumeGridQuantity::VolumeGridQuantity(std::string name_, VolumeGrid& curveNetwork_, bool dominates_)
    : QuantityS<VolumeGrid>(name_, curveNetwork_, dominates_) {}


VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                            DataType dataType_) {

  // TODO FIXME

  // VolumeGridScalarQuantity* q = new VolumeGridScalarQuantity(name, *this, data, dataType_);
  // addQuantity(q);
  // return q;
  return nullptr;
}

/*
VolumeGridVectorQuantity* VolumeGrid::addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& data,
                                                            VectorType dataType_) {
  VolumeGridVectorQuantity* q = new VolumeGridVectorQuantity(name, *this, data, dataType_);
  addQuantity(q);
  return q;
}
*/

/*
VolumeGridScalarIsosurface* VolumeGrid::addIsosurfaceQuantityImpl(std::string name, double isoLevel,
                                                                  const std::vector<double>& data) {
  VolumeGridScalarIsosurface* q = new VolumeGridScalarIsosurface(name, *this, data, isoLevel);
  addQuantity(q);
  q->setEnabled(true);
  return q;
}
*/

void VolumeGridQuantity::buildPointInfoGUI(size_t vInd) {}

VolumeGrid* registerVolumeGrid(std::string name, glm::uvec3 gridNodeDim, glm::vec3 bound_min, glm::vec3 bound_max) {
  VolumeGrid* s = new VolumeGrid(name, gridNodeDim, bound_min, bound_max);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

VolumeGrid* registerVolumeGrid(std::string name, size_t gridNodeDim, glm::vec3 bound_min, glm::vec3 bound_max) {
  return registerVolumeGrid(name, {gridNodeDim, gridNodeDim, gridNodeDim}, bound_min, bound_max);
}

} // namespace polyscope

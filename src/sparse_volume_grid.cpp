// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid.h"
#include "polyscope/sparse_volume_grid_color_quantity.h"
#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include "polyscope/pick.h"

#include "imgui.h"

namespace polyscope {

// Initialize statics
const std::string SparseVolumeGrid::structureTypeName = "Sparse Volume Grid";

SparseVolumeGrid::SparseVolumeGrid(std::string name, glm::vec3 origin_, glm::vec3 gridCellWidth_,
                                   std::vector<glm::ivec3> occupiedCells)
    : Structure(name, typeName()),

      // clang-format off
      // == managed quantities
      cellPositions(this, uniquePrefix() + "#cellPositions", cellPositionsData, std::bind(&SparseVolumeGrid::computeCellPositions, this)),
      cellIndices(this, uniquePrefix() + "#cellIndices", cellIndicesData, [](){/* do nothing, gets handled by computeCellPositions */}),

      origin(origin_), gridCellWidth(gridCellWidth_),

      // == persistent options
      color(               uniquePrefix() + "color",             getNextUniqueColor()),
      material(            uniquePrefix() + "material",          "clay"),
      cubeSizeFactor(      uniquePrefix() + "cubeSizeFactor",    0.f)
// clang-format on
{
  occupiedCellsData = std::move(occupiedCells);

  computeCellPositions();

  cullWholeElements.setPassive(true);
  updateObjectSpaceBounds();
}


void SparseVolumeGrid::computeCellPositions() {
  size_t n = occupiedCellsData.size();

  cellPositionsData.resize(n);
  cellIndicesData.resize(n);

  for (size_t i = 0; i < n; i++) {
    glm::ivec3 ijk = occupiedCellsData[i];
    cellPositionsData[i] = origin + (glm::vec3(ijk) + 0.5f) * gridCellWidth;
    cellIndicesData[i] = glm::uvec3(ijk); // cast to unsigned for GPU attribute
  }

  cellPositions.markHostBufferUpdated();
  cellIndices.markHostBufferUpdated();
}


void SparseVolumeGrid::buildCustomUI() {
  ImGui::Text("%llu cells", static_cast<unsigned long long>(nCells()));

  { // Color
    if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(color.get());
  }
}


void SparseVolumeGrid::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get());
  }

  // Shrinky effect
  if (ImGui::SliderFloat("Cell Shrink", &cubeSizeFactor.get(), 0.0, 1., "%.3f", ImGuiSliderFlags_Logarithmic)) {
    cubeSizeFactor.manuallyChanged();
    requestRedraw();
  }
}


void SparseVolumeGrid::draw() {
  if (!enabled.get()) return;

  // Ensure we have prepared buffers
  ensureRenderProgramPrepared();

  // Set program uniforms
  setStructureUniforms(*program);
  program->setUniform("u_gridSpacing", gridCellWidth);
  program->setUniform("u_cubeSizeFactor", 1.f - cubeSizeFactor.get());
  program->setUniform("u_baseColor", color.get());
  render::engine->setMaterialUniforms(*program, material.get());

  // Draw the actual grid
  render::engine->setBackfaceCull(true);
  program->draw();
}


void SparseVolumeGrid::drawDelayed() {
  if (!enabled.get()) return;
}


void SparseVolumeGrid::drawPick() {
  if (!isEnabled()) return;

  ensurePickProgramPrepared();

  // Set program uniforms
  setStructureUniforms(*pickProgram);
  pickProgram->setUniform("u_gridSpacing", gridCellWidth);
  pickProgram->setUniform("u_cubeSizeFactor", 1.f - cubeSizeFactor.get());
  pickProgram->setUniform("u_pickColor", pickColor);

  // Draw the actual grid
  render::engine->setBackfaceCull(true);
  pickProgram->draw();
}


void SparseVolumeGrid::drawPickDelayed() {
  if (!isEnabled()) return;
}


void SparseVolumeGrid::ensureRenderProgramPrepared() {
  if (program) return;

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(material.get(),
        addStructureRules({"SHADE_BASECOLOR"})
      )
  );
  // clang-format on

  program->setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  program->setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());

  render::engine->setMaterial(*program, material.get());
}


void SparseVolumeGrid::ensurePickProgramPrepared() {
  if (pickProgram) return;

  // clang-format off
  pickProgram = render::engine->requestShader(
      "GRIDCUBE",
      addStructureRules({"GRIDCUBE_CONSTANT_PICK"}),
      render::ShaderReplacementDefaults::Pick
  );
  // clang-format on

  pickProgram->setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  pickProgram->setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());

  if (globalPickConstant == INVALID_IND_64) {
    globalPickConstant = pick::requestPickBufferRange(this, 1);
    pickColor = pick::indToVec(static_cast<size_t>(globalPickConstant));
  }
}


void SparseVolumeGrid::updateObjectSpaceBounds() {

  if (cellPositionsData.empty()) {
    // no cells, degenerate bounds at origin
    objectSpaceBoundingBox = std::make_tuple(origin, origin);
    objectSpaceLengthScale = glm::length(gridCellWidth);
    return;
  }

  glm::vec3 bboxMin = cellPositionsData[0];
  glm::vec3 bboxMax = cellPositionsData[0];
  for (const glm::vec3& p : cellPositionsData) {
    bboxMin = glm::min(bboxMin, p);
    bboxMax = glm::max(bboxMax, p);
  }

  // Expand by half a cell width in each direction (positions are cell centers)
  glm::vec3 halfCell = 0.5f * gridCellWidth;
  bboxMin -= halfCell;
  bboxMax += halfCell;

  objectSpaceBoundingBox = std::make_tuple(bboxMin, bboxMax);
  objectSpaceLengthScale = glm::length(bboxMax - bboxMin);
}


std::string SparseVolumeGrid::typeName() { return structureTypeName; }


void SparseVolumeGrid::refresh() {
  Structure::refresh();

  program.reset();
  pickProgram.reset();
}


void SparseVolumeGrid::buildPickUI(const PickResult& rawResult) {

  // Determine which cell was clicked, CPU-side
  glm::vec3 localPos = (rawResult.position - origin) / gridCellWidth;

  // Find the cell index
  glm::ivec3 cellInd3{static_cast<int>(std::floor(localPos.x)), static_cast<int>(std::floor(localPos.y)),
                      static_cast<int>(std::floor(localPos.z))};

  ImGui::TextUnformatted(("Cell index: (" + std::to_string(cellInd3.x) + "," + std::to_string(cellInd3.y) + "," +
                          std::to_string(cellInd3.z) + ")")
                             .c_str());

  glm::vec3 cellCenter = origin + (glm::vec3(cellInd3) + 0.5f) * gridCellWidth;
  std::stringstream buffer;
  buffer << cellCenter;
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());
}


// === Option getters and setters

SparseVolumeGrid* SparseVolumeGrid::setColor(glm::vec3 val) {
  color = val;
  requestRedraw();
  return this;
}
glm::vec3 SparseVolumeGrid::getColor() { return color.get(); }

SparseVolumeGrid* SparseVolumeGrid::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string SparseVolumeGrid::getMaterial() { return material.get(); }

SparseVolumeGrid* SparseVolumeGrid::setCubeSizeFactor(double newVal) {
  cubeSizeFactor = newVal;
  requestRedraw();
  return this;
}
double SparseVolumeGrid::getCubeSizeFactor() { return cubeSizeFactor.get(); }


// === Register functions (non-template overload)

SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const std::vector<glm::ivec3>& occupiedCells) {
  checkInitialized();

  SparseVolumeGrid* s = new SparseVolumeGrid(name, origin, gridCellWidth, occupiedCells);

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

// === Quantity base class

SparseVolumeGridQuantity::SparseVolumeGridQuantity(std::string name_, SparseVolumeGrid& grid_, bool dominates_)
    : Quantity(name_, grid_, dominates_), parent(grid_) {}


// === Set cell geometry attributes helper

void SparseVolumeGrid::setCellGeometryAttributes(render::ShaderProgram& p) {
  p.setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  p.setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());
}


// === Quantity adders

SparseVolumeGridScalarQuantity* SparseVolumeGrid::addCellScalarQuantityImpl(std::string name,
                                                                            const std::vector<float>& data,
                                                                            DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridScalarQuantity* q = new SparseVolumeGridScalarQuantity(name, *this, data, type);
  addQuantity(q);
  return q;
}

SparseVolumeGridScalarQuantity* SparseVolumeGrid::addNodeScalarQuantityImpl(
    std::string name, const std::vector<glm::ivec3>& nodeIndices, const std::vector<float>& nodeValues,
    DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridScalarQuantity* q = new SparseVolumeGridScalarQuantity(name, *this, nodeIndices, nodeValues, type);
  addQuantity(q);
  return q;
}

SparseVolumeGridColorQuantity* SparseVolumeGrid::addCellColorQuantityImpl(std::string name,
                                                                          const std::vector<glm::vec3>& colors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridColorQuantity* q = new SparseVolumeGridColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

SparseVolumeGridColorQuantity* SparseVolumeGrid::addNodeColorQuantityImpl(
    std::string name, const std::vector<glm::ivec3>& nodeIndices, const std::vector<glm::vec3>& nodeColors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridColorQuantity* q = new SparseVolumeGridColorQuantity(name, *this, nodeIndices, nodeColors);
  addQuantity(q);
  return q;
}


} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid.h"
#include "polyscope/sparse_volume_grid_color_quantity.h"
#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include "polyscope/pick.h"

#include "imgui.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>

namespace polyscope {

// Initialize statics
const std::string SparseVolumeGrid::structureTypeName = "Sparse Volume Grid";

namespace {

// comparator for sorting int3 vecs
bool ivec3Less(const glm::ivec3& a, const glm::ivec3& b) {
  if (a.x != b.x) return a.x < b.x;
  if (a.y != b.y) return a.y < b.y;
  return a.z < b.z;
};
} // namespace

SparseVolumeGrid::SparseVolumeGrid(std::string name, glm::vec3 origin_, glm::vec3 gridCellWidth_,
                                   std::vector<glm::ivec3> occupiedCells)
    : Structure(name, typeName()),
      // clang-format off
      // == managed quantities
      cellPositions(this, uniquePrefix() + "#cellPositions", cellPositionsData, std::bind(&SparseVolumeGrid::computeCellPositions, this)),
      cellIndices(this, uniquePrefix() + "#cellIndices", cellIndicesData, [](){/* do nothing, gets handled by computeCellPositions */}),
      cornerNodeInds{
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds0", cornerNodeIndsData[0]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds1", cornerNodeIndsData[1]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds2", cornerNodeIndsData[2]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds3", cornerNodeIndsData[3]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds4", cornerNodeIndsData[4]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds5", cornerNodeIndsData[5]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds6", cornerNodeIndsData[6]),
        render::ManagedBuffer<uint32_t>(this, uniquePrefix() + "#cornerNodeInds7", cornerNodeIndsData[7]),
      },

      origin(origin_), gridCellWidth(gridCellWidth_),

      // == persistent options
      color(               uniquePrefix() + "color",             getNextUniqueColor()),
      edgeWidth(           uniquePrefix() + "edgeWidth",         0.f),
      edgeColor(           uniquePrefix() + "edgeColor",         glm::vec3{0., 0., 0.}), 
      material(            uniquePrefix() + "material",          "clay"),
      cubeSizeFactor(      uniquePrefix() + "cubeSizeFactor",    0.f)
    {
  // clang-format on
  occupiedCellsData = std::move(occupiedCells);

  checkForDuplicateCells();
  computeCellPositions();

  cullWholeElements.setPassive(true);
  updateObjectSpaceBounds();
}

void SparseVolumeGrid::checkForDuplicateCells() {

  // sort occupied cells and check for duplicates
  std::vector<glm::ivec3> sortedCells = occupiedCellsData;
  std::sort(sortedCells.begin(), sortedCells.end(), [](const glm::ivec3& a, const glm::ivec3& b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
  });
  for (size_t i = 1; i < sortedCells.size(); i++) {
    if (sortedCells[i] == sortedCells[i - 1]) {
      error("[Polyscope] sparse volume grid " + name + " has repeated cell (" + std::to_string(sortedCells[i].x) + "," +
            std::to_string(sortedCells[i].y) + "," + std::to_string(sortedCells[i].z) + ")");
    }
  }
}

void SparseVolumeGrid::computeCellPositions() {
  size_t n = occupiedCellsData.size();

  cellPositionsData.resize(n);
  cellIndicesData.resize(n);

  for (size_t i = 0; i < n; i++) {
    glm::ivec3 ijk = occupiedCellsData[i];
    cellPositionsData[i] = origin + (glm::vec3(ijk) + 0.5f) * gridCellWidth;
    cellIndicesData[i] = ijk;
  }

  cellPositions.markHostBufferUpdated();
  cellIndices.markHostBufferUpdated();
}


void SparseVolumeGrid::ensureHaveCornerNodeIndices() {
  if (haveCornerNodeIndices) return;
  computeCornerNodeIndices();
  haveCornerNodeIndices = true;
}

void SparseVolumeGrid::computeCornerNodeIndices() {
  size_t n = occupiedCellsData.size();

  // Collect all node ivec3 from all cells
  std::vector<glm::ivec3> allNodes;
  allNodes.reserve(n * 8);
  for (size_t i = 0; i < n; i++) {
    glm::ivec3 ci = occupiedCellsData[i];
    for (int dx = 0; dx < 2; dx++)
      for (int dy = 0; dy < 2; dy++)
        for (int dz = 0; dz < 2; dz++) allNodes.push_back(glm::ivec3(ci.x + dx, ci.y + dy, ci.z + dz));
  }

  // Sort and deduplicate to get canonical order
  std::sort(allNodes.begin(), allNodes.end(), ivec3Less);
  allNodes.erase(std::unique(allNodes.begin(), allNodes.end()), allNodes.end());
  canonicalNodeIndsData = std::move(allNodes);

  // Build a hashmap from node ivec3 to canonical index
  auto ivec3Hash = [](const glm::ivec3& v) {
    size_t h = std::hash<int>()(v.x);
    h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  };
  auto ivec3Equal = [](const glm::ivec3& a, const glm::ivec3& b) { return a == b; };
  std::unordered_map<glm::ivec3, uint32_t, decltype(ivec3Hash), decltype(ivec3Equal)> nodeToIndex(
      canonicalNodeIndsData.size(), ivec3Hash, ivec3Equal);
  for (size_t i = 0; i < canonicalNodeIndsData.size(); i++) {
    nodeToIndex[canonicalNodeIndsData[i]] = static_cast<uint32_t>(i);
  }

  // Build corner index buffers using hashmap lookup
  for (int c = 0; c < 8; c++) {
    cornerNodeIndsData[c].resize(n);
  }

  for (size_t i = 0; i < n; i++) {
    glm::ivec3 ci = occupiedCellsData[i];
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          int c = dx * 4 + dy * 2 + dz;
          glm::ivec3 nodeIjk(ci.x + dx, ci.y + dy, ci.z + dz);
          cornerNodeIndsData[c][i] = nodeToIndex[nodeIjk];
        }
      }
    }
  }

  for (int c = 0; c < 8; c++) {
    cornerNodeInds[c].markHostBufferUpdated();
  }
}


void SparseVolumeGrid::buildCustomUI() {
  ImGui::Text("%llu cells", static_cast<unsigned long long>(nCells()));

  { // Color
    if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(color.get());
  }

  { // Edge options
    ImGui::SameLine();
    ImGui::PushItemWidth(100 * options::uiScale);
    if (edgeWidth.get() == 0.) {
      bool showEdges = false;
      if (ImGui::Checkbox("Edges", &showEdges)) {
        setEdgeWidth(1.);
      }
    } else {
      bool showEdges = true;
      if (ImGui::Checkbox("Edges", &showEdges)) {
        setEdgeWidth(0.);
      }

      // Edge color
      ImGui::PushItemWidth(100 * options::uiScale);
      if (ImGui::ColorEdit3("Edge Color", &edgeColor.get()[0], ImGuiColorEditFlags_NoInputs))
        setEdgeColor(edgeColor.get());
      ImGui::PopItemWidth();

      // Edge width
      ImGui::SameLine();
      ImGui::PushItemWidth(75 * options::uiScale);
      if (ImGui::SliderFloat("Width", &edgeWidth.get(), 0.001, 2.)) {
        // NOTE: this intentionally circumvents the setEdgeWidth() setter to avoid repopulating the buffer as the
        // slider is dragged---otherwise we repopulate the buffer on every change. This is a
        // lazy solution instead of better state/buffer management.
        edgeWidth.manuallyChanged();
        requestRedraw();
      }
      ImGui::PopItemWidth();
    }
    ImGui::PopItemWidth();
  }
}


void SparseVolumeGrid::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get());
  }

  // Shrinky effect
  ImGui::PushItemWidth(150 * options::uiScale);
  if (ImGui::SliderFloat("Cell Shrink", &cubeSizeFactor.get(), 0.0, 1., "%.3f", ImGuiSliderFlags_Logarithmic)) {
    cubeSizeFactor.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}


void SparseVolumeGrid::draw() {
  if (!enabled.get()) return;

  if (dominantQuantity == nullptr) {
    // if there is no dominant quantity, the structure handles drawing

    // Ensure we have prepared buffers
    ensureRenderProgramPrepared();

    // Set program uniforms
    setSparseVolumeGridUniforms(*program);
    program->setUniform("u_baseColor", color.get());

    // Draw the actual grid
    program->draw();
    render::engine->setBackfaceCull(true);
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}


void SparseVolumeGrid::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}


void SparseVolumeGrid::drawPick() {
  if (!isEnabled()) return;

  ensurePickProgramPrepared();

  // Set program uniforms
  setSparseVolumeGridUniforms(*pickProgram, true);

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
        addSparseGridShaderRules({"SHADE_BASECOLOR"})
      )
  );
  // clang-format on

  program->setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  program->setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());

  render::engine->setMaterial(*program, material.get());
}


void SparseVolumeGrid::ensurePickProgramPrepared() {
  if (pickProgram) return;

  // Request pick indices
  size_t pickCount = nCells();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // clang-format off
  pickProgram = render::engine->requestShader(
      "GRIDCUBE",
      addSparseGridShaderRules({"GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR"}, true),
      render::ShaderReplacementDefaults::Pick
  );
  // clang-format on

  // Fill color buffer with packed point indices
  std::vector<glm::vec3> pickColors(pickCount);
  for (size_t i = 0; i < pickCount; i++) {
    pickColors[i] = pick::indToVec(i + pickStart);
  }
  pickProgram->setAttribute("a_color", pickColors);


  pickProgram->setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  pickProgram->setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());
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


SparseVolumeGridPickResult SparseVolumeGrid::interpretPickResult(const PickResult& rawResult) {
  const float nodePickRad = 0.8; // threshold to click node, measured in a [-1,1] cube

  if (rawResult.structure != this) {
    exception("called interpretPickResult(), but the pick result is not from this structure");
  }

  SparseVolumeGridPickResult result;
  result.cellIndex = glm::vec3{0, 0, 0};
  result.cellFlatIndex = INVALID_IND_64;
  result.nodeIndex = glm::vec3{0, 0, 0};

  glm::vec3 localPos = (rawResult.position - origin) / gridCellWidth;

  // Find the cell index
  glm::ivec3 cellInd3 = cellIndicesData[rawResult.localIndex];

  // Fractional position within cell [0,1]
  glm::vec3 fractional = localPos - glm::vec3(cellInd3);

  // Local coordinates in [-1,1] within the scaled cell
  glm::vec3 coordModShift = 2.f * fractional - 1.f;
  float csf = cubeSizeFactor.get();
  glm::vec3 coordLocal = (csf < 1.f) ? (coordModShift / (1.f - csf)) : coordModShift;
  float distFromCorner = glm::length(1.f - glm::abs(coordLocal));

  bool doPickNodes;
  if (nodesHaveBeenUsed) {
    doPickNodes = distFromCorner < nodePickRad;
  } else {
    doPickNodes = false;
  }

  if (doPickNodes) {
    // return a node pick

    ensureHaveCornerNodeIndices();

    // find the nearest corner
    int cornerDx = fractional.x > 0.5f ? 1 : 0;
    int cornerDy = fractional.y > 0.5f ? 1 : 0;
    int cornerDz = fractional.z > 0.5f ? 1 : 0;
    glm::ivec3 nodeInd3(cellInd3.x + cornerDx - 1, cellInd3.y + cornerDy - 1, cellInd3.z + cornerDz - 1);

    result.elementType = SparseVolumeGridElement::NODE;
    result.nodeIndex = nodeInd3;

  } else {
    // return a cell pick

    result.elementType = SparseVolumeGridElement::CELL;
    result.cellIndex = cellInd3;
    result.cellFlatIndex = rawResult.localIndex;
  }

  return result;
}


void SparseVolumeGrid::buildPickUI(const PickResult& rawResult) {

  SparseVolumeGridPickResult result = interpretPickResult(rawResult);

  switch (result.elementType) {
  case SparseVolumeGridElement::NODE: {
    buildNodeInfoGUI(result);
    break;
  }
  case SparseVolumeGridElement::CELL: {
    buildCellInfoGUI(result);
    break;
  }
  };
}


void SparseVolumeGrid::buildCellInfoGUI(const SparseVolumeGridPickResult& result) {

  glm::ivec3 cellInd3 = result.cellIndex;
  size_t flatInd = result.cellFlatIndex;

  ImGui::TextUnformatted(("Cell index: (" + std::to_string(cellInd3.x) + "," + std::to_string(cellInd3.y) + "," +
                          std::to_string(cellInd3.z) + ")")
                             .c_str());
  if (flatInd != INVALID_IND_64) {
    ImGui::TextUnformatted(("Cell #" + std::to_string(flatInd)).c_str());
  }

  glm::vec3 cellCenter = origin + (glm::vec3(cellInd3) + 0.5f) * gridCellWidth;
  std::stringstream buffer;
  buffer << cellCenter;
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  if (flatInd != INVALID_IND_64) {
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Indent(20.);

    // Build GUI to show the quantities
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
    for (auto& x : quantities) {
      SparseVolumeGridQuantity* q = static_cast<SparseVolumeGridQuantity*>(x.second.get());
      q->buildCellInfoGUI(flatInd);
    }

    ImGui::Indent(-20.);
  }
}


void SparseVolumeGrid::buildNodeInfoGUI(const SparseVolumeGridPickResult& result) {

  glm::ivec3 nodeInd3 = result.nodeIndex;

  ImGui::TextUnformatted(("Node index: (" + std::to_string(nodeInd3.x) + "," + std::to_string(nodeInd3.y) + "," +
                          std::to_string(nodeInd3.z) + ")")
                             .c_str());

  // Find canonical node index
  ensureHaveCornerNodeIndices();
  auto it = std::lower_bound(canonicalNodeIndsData.begin(), canonicalNodeIndsData.end(), nodeInd3, ivec3Less);
  size_t canonicalInd = INVALID_IND_64;
  if (it != canonicalNodeIndsData.end() && *it == nodeInd3) {
    canonicalInd = static_cast<size_t>(it - canonicalNodeIndsData.begin());
    ImGui::TextUnformatted(("Node #" + std::to_string(canonicalInd)).c_str());
  }

  glm::vec3 nodePos = origin + glm::vec3(nodeInd3 + 1) * gridCellWidth;
  std::stringstream buffer;
  buffer << nodePos;
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  if (canonicalInd != INVALID_IND_64) {
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Indent(20.);

    // Build GUI to show the quantities
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
    for (auto& x : quantities) {
      SparseVolumeGridQuantity* q = static_cast<SparseVolumeGridQuantity*>(x.second.get());
      q->buildNodeInfoGUI(canonicalInd);
    }

    ImGui::Indent(-20.);
  }
}


size_t SparseVolumeGrid::findCellFlatIndex(glm::ivec3 cellInd3) {
  for (size_t i = 0; i < occupiedCellsData.size(); i++) {
    if (occupiedCellsData[i] == cellInd3) return i;
  }
  return INVALID_IND_64;
}

size_t SparseVolumeGrid::findNodeFlatIndex(glm::ivec3 nodeInd3) {

  if (!haveCornerNodeIndices) {
    error("findFlatNodeIndex requires that node indices have been prepared");
    return INVALID_IND_64;
  }

  // binary search
  auto it = std::lower_bound(canonicalNodeIndsData.begin(), canonicalNodeIndsData.end(), nodeInd3, ivec3Less);
  size_t canonicalInd = INVALID_IND_64;
  if (it != canonicalNodeIndsData.end() && *it == nodeInd3) {
    return static_cast<size_t>(it - canonicalNodeIndsData.begin());
  }

  return INVALID_IND_64;
}

void SparseVolumeGrid::markNodesAsUsed() { nodesHaveBeenUsed = true; }


// === Option getters and setters

SparseVolumeGrid* SparseVolumeGrid::setColor(glm::vec3 val) {
  color = val;
  requestRedraw();
  return this;
}
glm::vec3 SparseVolumeGrid::getColor() { return color.get(); }

SparseVolumeGrid* SparseVolumeGrid::setEdgeWidth(double newVal) {
  double oldEdgeWidth = edgeWidth.get();
  edgeWidth = newVal;
  if ((oldEdgeWidth != 0) != (newVal != 0)) {
    // if it changed to/from zero, we disabled/enabled edgges, and need a new program
    refresh();
  }
  requestRedraw();
  return this;
}
double SparseVolumeGrid::getEdgeWidth() { return edgeWidth.get(); }

SparseVolumeGrid* SparseVolumeGrid::setEdgeColor(glm::vec3 val) {
  edgeColor = val;
  requestRedraw();
  return this;
}
glm::vec3 SparseVolumeGrid::getEdgeColor() { return edgeColor.get(); }

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


void SparseVolumeGrid::setCellGeometryAttributes(render::ShaderProgram& p) {
  p.setAttribute("a_cellPosition", cellPositions.getRenderAttributeBuffer());
  p.setAttribute("a_cellInd", cellIndices.getRenderAttributeBuffer());
}

std::vector<std::string> SparseVolumeGrid::addSparseGridShaderRules(std::vector<std::string> initRules, bool pickOnly) {
  if (!pickOnly) {
    if (getEdgeWidth() > 0) {
      initRules.push_back("GRIDCUBE_WIREFRAME");
      initRules.push_back("MESH_WIREFRAME");
    }
  }

  if (wantsCullPosition()) {
    initRules.push_back("GRIDCUBE_CULLPOS_FROM_CENTER");
  }

  return addStructureRules(initRules);
}

void SparseVolumeGrid::setSparseVolumeGridUniforms(render::ShaderProgram& p, bool pickOnly) {
  setStructureUniforms(p);
  p.setUniform("u_gridSpacing", gridCellWidth);
  p.setUniform("u_cubeSizeFactor", 1.f - cubeSizeFactor.get());
  if (!pickOnly) {
    if (getEdgeWidth() > 0) {
      float edgeMult = 2.0f; // need to be a bit thicker to look nice for these
      p.setUniform("u_edgeWidth", edgeMult * getEdgeWidth() * render::engine->getCurrentPixelScaling());
      p.setUniform("u_edgeColor", getEdgeColor());
    }

    render::engine->setMaterialUniforms(p, material.get());
  }
}

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

// Default implementations
void SparseVolumeGridQuantity::buildCellInfoGUI(size_t cellInd) {}
void SparseVolumeGridQuantity::buildNodeInfoGUI(size_t nodeInd) {}

// === Quantity adders

SparseVolumeGridCellScalarQuantity*
SparseVolumeGrid::addCellScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridCellScalarQuantity* q = new SparseVolumeGridCellScalarQuantity(name, *this, data, type);
  addQuantity(q);
  return q;
}

SparseVolumeGridNodeScalarQuantity*
SparseVolumeGrid::addNodeScalarQuantityImpl(std::string name, const std::vector<glm::ivec3>& nodeIndices,
                                            const std::vector<float>& nodeValues, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridNodeScalarQuantity* q =
      new SparseVolumeGridNodeScalarQuantity(name, *this, nodeIndices, nodeValues, type);
  addQuantity(q);
  markNodesAsUsed();
  return q;
}

SparseVolumeGridCellColorQuantity* SparseVolumeGrid::addCellColorQuantityImpl(std::string name,
                                                                              const std::vector<glm::vec3>& colors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridCellColorQuantity* q = new SparseVolumeGridCellColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

SparseVolumeGridNodeColorQuantity*
SparseVolumeGrid::addNodeColorQuantityImpl(std::string name, const std::vector<glm::ivec3>& nodeIndices,
                                           const std::vector<glm::vec3>& nodeColors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SparseVolumeGridNodeColorQuantity* q = new SparseVolumeGridNodeColorQuantity(name, *this, nodeIndices, nodeColors);
  addQuantity(q);
  markNodesAsUsed();
  return q;
}


} // namespace polyscope

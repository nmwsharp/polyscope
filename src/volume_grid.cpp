// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_grid.h"

#include "polyscope/pick.h"

#include "imgui.h"

namespace polyscope {

// Initialize statics
const std::string VolumeGrid::structureTypeName = "Volume Grid";

VolumeGrid::VolumeGrid(std::string name, glm::uvec3 gridNodeDim_, glm::vec3 boundMin_, glm::vec3 boundMax_)
    : QuantityStructure<VolumeGrid>(name, typeName()),

      // clang-format off
      // == managed quantities
      gridPlaneReferencePositions(this, uniquePrefix() +  "#gridPlaneReferencePositions",     gridPlaneReferencePositionsData,    std::bind(&VolumeGrid::computeGridPlaneReferenceGeometry, this)),
      gridPlaneReferenceNormals(this, uniquePrefix() +    "#gridPlaneReferenceNormals",       gridPlaneReferenceNormalsData,      [](){/* do nothing, gets handled by position func */} ),
      gridPlaneAxisInds(this, uniquePrefix() +            "#gridPlaneAxisInds",               gridPlaneAxisIndsData,              [](){/* do nothing, gets handled by position func */} ),

       gridNodeDim(gridNodeDim_), gridCellDim(gridNodeDim_ - 1u), boundMin(boundMin_), boundMax(boundMax_),

      // == persistent options
      color(                  uniquePrefix() + "color",             getNextUniqueColor()),
      edgeColor(              uniquePrefix() + "edgeColor",         glm::vec3{0., 0., 0.}), 
      material(               uniquePrefix() + "material",          "clay"),
      edgeWidth(              uniquePrefix() + "edgeWidth",         0.f),
      cubeSizeFactor(         uniquePrefix() + "cubeSizeFactor",    0.f)
// clang-format on
{
  cullWholeElements.setPassive(true);
  updateObjectSpaceBounds();
}


void VolumeGrid::buildCustomUI() {
  ImGui::Text("node dim (%lld, %lld, %lld)", static_cast<long long int>(gridNodeDim.x),
              static_cast<long long int>(gridNodeDim.y), static_cast<long long int>(gridNodeDim.z));

  // these all take up too much space
  // ImGui::TextUnformatted(("min: " + to_string_short(boundMin)).c_str());
  // ImGui::TextUnformatted(("max: " + to_string_short(boundMax)).c_str());
  // ImGui::TextUnformatted((to_string_short(boundMin) + " -- " + to_string_short(boundMax)).c_str());

  { // Colors
    if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(color.get());
    ImGui::SameLine();
  }


  { // Edge options
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
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
      ImGui::PushItemWidth(100);
      if (ImGui::ColorEdit3("Edge Color", &edgeColor.get()[0], ImGuiColorEditFlags_NoInputs))
        setEdgeColor(edgeColor.get());
      ImGui::PopItemWidth();

      // Edge width
      ImGui::SameLine();
      ImGui::PushItemWidth(75);
      if (ImGui::SliderFloat("Width", &edgeWidth.get(), 0.001, 2.)) {
        // NOTE: this intentionally circumvents the setEdgeWidth() setter to avoid repopulating the buffer as the
        // slider is dragged---otherwise we repopulate the buffer on every change, which mostly works fine. This is a
        // lazy solution instead of better state/buffer management. setEdgeWidth(getEdgeWidth());
        edgeWidth.manuallyChanged();
        requestRedraw();
      }
      ImGui::PopItemWidth();
    }
    ImGui::PopItemWidth();
  }
}


void VolumeGrid::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }

  // Shrinky effect
  if (ImGui::SliderFloat("Cell Shrink", &cubeSizeFactor.get(), 0.0, 1., "%.3f", ImGuiSliderFlags_Logarithmic)) {
    cubeSizeFactor.manuallyChanged();
    requestRedraw();
  }
}

void VolumeGrid::draw() {
  if (!enabled.get()) return;

  // Right now none of this class supports cullWholeElements = false, so just always force it to true
  if (!getCullWholeElements()) {
    setCullWholeElements(true);
  }

  // If there is no dominant quantity, then this class is responsible for the grid
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    ensureGridCubeRenderProgramPrepared();

    // Set program uniforms
    setStructureUniforms(*program);
    setGridCubeUniforms(*program);
    program->setUniform("u_baseColor", color.get());
    render::engine->setMaterialUniforms(*program, material.get());

    // Draw the actual grid
    render::engine->setBackfaceCull(true);
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
  if (!isEnabled()) {
    return;
  }

  // only draw pick if the grid is actually being draw
  if (dominantQuantity != nullptr) {
    VolumeGridQuantity* g = dynamic_cast<VolumeGridQuantity*>(dominantQuantity);
    if (g && !g->isDrawingGridcubes()) {
      return;
    }
  }

  ensureGridCubePickProgramPrepared();

  // Set program uniforms
  setStructureUniforms(*pickProgram);
  setGridCubeUniforms(*pickProgram, false);
  pickProgram->setUniform("u_pickColor", pickColor);

  // Draw the actual grid
  render::engine->setBackfaceCull(true);
  pickProgram->draw();
}

std::vector<std::string> VolumeGrid::addGridCubeRules(std::vector<std::string> initRules, bool withShade) {
  initRules = addStructureRules(initRules);

  if (withShade) {
    if (getEdgeWidth() > 0) {
      initRules.push_back("GRIDCUBE_WIREFRAME");
      // initRules.push_back("WIREFRAME_SIMPLE");
      initRules.push_back("MESH_WIREFRAME");
    }
  }

  if (wantsCullPosition()) {
    initRules.push_back("GRIDCUBE_CULLPOS_FROM_CENTER");
  }

  return initRules;
}

void VolumeGrid::setGridCubeUniforms(render::ShaderProgram& p, bool withShade) {

  p.setUniform("u_boundMin", boundMin);
  p.setUniform("u_boundMax", boundMax);
  p.setUniform("u_cubeSizeFactor", 1.f - cubeSizeFactor.get());
  p.setUniform("u_gridSpacingReference", gridSpacingReference());

  if (withShade) {

    if (getEdgeWidth() > 0) {
      p.setUniform("u_edgeWidth", getEdgeWidth() * render::engine->getCurrentPixelScaling());
      p.setUniform("u_edgeColor", getEdgeColor());
    }
  }
}

void VolumeGrid::ensureGridCubeRenderProgramPrepared() {
  // If already prepared, do nothing
  if (program) return;

  // clang-format off
  program = render::engine->requestShader( "GRIDCUBE_PLANE", 
      render::engine->addMaterialRules(material.get(),
        addGridCubeRules(
          {"SHADE_BASECOLOR"}, 
        true)
      )
  );
  // clang-format on

  program->setAttribute("a_referencePosition", gridPlaneReferencePositions.getRenderAttributeBuffer());
  program->setAttribute("a_referenceNormal", gridPlaneReferenceNormals.getRenderAttributeBuffer());
  program->setAttribute("a_axisInd", gridPlaneAxisInds.getRenderAttributeBuffer());

  render::engine->setMaterial(*program, material.get());
}

void VolumeGrid::ensureGridCubePickProgramPrepared() {

  // If already prepared, do nothing
  if (pickProgram) return;

  // clang-format off
  pickProgram = render::engine->requestShader(
      "GRIDCUBE_PLANE", 
      addGridCubeRules({"GRIDCUBE_CONSTANT_PICK"}, false), 
      render::ShaderReplacementDefaults::Pick
  );
  // clang-format on


  pickProgram->setAttribute("a_referencePosition", gridPlaneReferencePositions.getRenderAttributeBuffer());
  pickProgram->setAttribute("a_referenceNormal", gridPlaneReferenceNormals.getRenderAttributeBuffer());
  pickProgram->setAttribute("a_axisInd", gridPlaneAxisInds.getRenderAttributeBuffer());


  if (globalPickConstant == INVALID_IND_64) {
    // request a pick range

    // NOTE: Picking for this structure works a bit differently than others.
    //
    // The usual approach of packing a 64bit in to 3 floats doesn't play nice
    // with the way the grid is implicitly represented: we would need to do all the
    // packing logic in the shader.
    //
    // Instead, we only shade with a single constant pick ind, then compute which
    // element was actually clicked CPU-side afterwards

    globalPickConstant = pick::requestPickBufferRange(this, 1);
    size_t cellGlobalPickIndStart = globalPickConstant + nNodes();
    pickColor = pick::indToVec(static_cast<size_t>(globalPickConstant));
  }
}


void VolumeGrid::updateObjectSpaceBounds() {
  objectSpaceBoundingBox = std::make_tuple(boundMin, boundMax);
  objectSpaceLengthScale = glm::length(boundMax - boundMin);
}

std::string VolumeGrid::typeName() { return structureTypeName; }

void VolumeGrid::refresh() {
  QuantityStructure<VolumeGrid>::refresh(); // call base class version, which refreshes quantities

  program.reset();
  pickProgram.reset();
}

void VolumeGrid::setVolumeGridUniforms(render::ShaderProgram& p) {}


void VolumeGrid::computeGridPlaneReferenceGeometry() {

  // NOTE: This slightly abuses the ManagedBuffer 'compute()' func,
  // by computing the data for multiple buffers with one function.
  // For now at least, it will work fine.

  // Geometry is defined in the reference [0,1] cube

  gridPlaneReferencePositions.data.clear();
  gridPlaneReferenceNormals.data.clear();
  gridPlaneAxisInds.data.clear();

  auto addPlane = [&](std::array<glm::vec3, 4> corners, glm::vec3 normal, uint32_t axInd) {
    // first triangle
    gridPlaneReferencePositions.data.push_back(corners[0]);
    gridPlaneReferencePositions.data.push_back(corners[1]);
    gridPlaneReferencePositions.data.push_back(corners[2]);
    for (int32_t j = 0; j < 3; j++) gridPlaneReferenceNormals.data.push_back(normal);
    for (int32_t j = 0; j < 3; j++) gridPlaneAxisInds.data.push_back(axInd);

    // second triangle
    gridPlaneReferencePositions.data.push_back(corners[1]);
    gridPlaneReferencePositions.data.push_back(corners[3]);
    gridPlaneReferencePositions.data.push_back(corners[2]);
    for (int32_t j = 0; j < 3; j++) gridPlaneReferenceNormals.data.push_back(normal);
    for (int32_t j = 0; j < 3; j++) gridPlaneAxisInds.data.push_back(axInd);
  };

  // The planes are intentionally added in order such that the outermost planes come first, and we don't massively
  // overshade from back to front. Note that fthe first look runs backwards.

  // Forward facing planes
  for (uint32_t d = 0; d < 3; d++) { // x/y/z dimension (plane is perpendicular)
    for (int32_t i = (int32_t)gridCellDim[d] - 1; i >= 0; i--) {

      float t = (static_cast<float>(i) + 1) / (gridCellDim[d]);

      // clang-format off
      glm::vec3 ll{0.f, 0.f, 0.f}; ll[(d+1)%3] = 0.f; ll[(d+2)%3] = 0.f; ll[d] = t;
      glm::vec3 lu{0.f, 0.f, 0.f}; lu[(d+1)%3] = 1.f; lu[(d+2)%3] = 0.f; lu[d] = t;
      glm::vec3 ul{0.f, 0.f, 0.f}; ul[(d+1)%3] = 0.f; ul[(d+2)%3] = 1.f; ul[d] = t;
      glm::vec3 uu{0.f, 0.f, 0.f}; uu[(d+1)%3] = 1.f; uu[(d+2)%3] = 1.f; uu[d] = t;

      glm::vec3 n{0.f, 0.f, 0.f}; n[d] = 1.f;
      // clang-format on

      addPlane({ll, lu, ul, uu}, n, i);
    }
  }

  // Backward facing planes
  for (uint32_t d = 0; d < 3; d++) { // x/y/z dimension (plane is perpendicular)
    for (int32_t i = 0; i < (int32_t)gridCellDim[d]; i++) {

      float t = (static_cast<float>(i)) / (gridCellDim[d]);

      // clang-format off
      glm::vec3 ll{0.f, 0.f, 0.f}; ll[(d+1)%3] = 0.f; ll[(d+2)%3] = 0.f; ll[d] = t;
      glm::vec3 lu{0.f, 0.f, 0.f}; lu[(d+1)%3] = 1.f; lu[(d+2)%3] = 0.f; lu[d] = t;
      glm::vec3 ul{0.f, 0.f, 0.f}; ul[(d+1)%3] = 0.f; ul[(d+2)%3] = 1.f; ul[d] = t;
      glm::vec3 uu{0.f, 0.f, 0.f}; uu[(d+1)%3] = 1.f; uu[(d+2)%3] = 1.f; uu[d] = t;

      glm::vec3 n{0.f, 0.f, 0.f}; n[d] = -1.f;
      // clang-format on

      addPlane({ul, uu, ll, lu}, n, i); // winding is opposite here
    }
  }


  gridPlaneReferencePositions.markHostBufferUpdated();
  gridPlaneReferenceNormals.markHostBufferUpdated();
  gridPlaneAxisInds.markHostBufferUpdated();
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

VolumeGrid* VolumeGrid::setCubeSizeFactor(double newVal) {
  cubeSizeFactor = newVal;
  requestRedraw();
  return this;
}
double VolumeGrid::getCubeSizeFactor() { return cubeSizeFactor.get(); }

// === Register functions


VolumeGridQuantity::VolumeGridQuantity(std::string name_, VolumeGrid& curveNetwork_, bool dominates_)
    : QuantityS<VolumeGrid>(name_, curveNetwork_, dominates_) {}


VolumeGridNodeScalarQuantity* VolumeGrid::addNodeScalarQuantityImpl(std::string name, const std::vector<float>& data,
                                                                    DataType dataType_) {

  checkForQuantityWithNameAndDeleteOrError(name);
  VolumeGridNodeScalarQuantity* q = new VolumeGridNodeScalarQuantity(name, *this, data, dataType_);
  addQuantity(q);
  markNodesAsUsed();
  return q;
}

VolumeGridCellScalarQuantity* VolumeGrid::addCellScalarQuantityImpl(std::string name, const std::vector<float>& data,
                                                                    DataType dataType_) {

  checkForQuantityWithNameAndDeleteOrError(name);
  VolumeGridCellScalarQuantity* q = new VolumeGridCellScalarQuantity(name, *this, data, dataType_);
  addQuantity(q);
  markCellsAsUsed();
  return q;
}

void VolumeGrid::markNodesAsUsed() { nodesHaveBeenUsed = true; }

void VolumeGrid::markCellsAsUsed() { cellsHaveBeenUsed = true; }


void VolumeGrid::buildPickUI(size_t localPickID) {

  // See note in ensurePickProgramPrepared().
  // Picking for this structure works different, and identifies which element with a depth query CPU side.

  float nodePickRad = 0.8; // measured in a [-1,1] cube

  ImGuiIO& io = ImGui::GetIO();
  glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
  glm::vec3 pickPos = view::screenCoordsToWorldPosition(screenCoords);
  glm::vec3 localPickPos = (pickPos - boundMin) / (boundMax - boundMin);
  localPickPos = clamp(localPickPos, glm::vec3(0.), glm::vec3(1.)); // on [0,1.]

  // NOTE: this logic is duplicated with shader
  glm::vec3 coordUnit = localPickPos / gridSpacingReference();
  glm::vec3 coordMod = mod(coordUnit, 1.f);
  glm::vec3 coordModShift = 2.f * coordMod - 1.f;
  glm::vec3 coordLocal = coordModShift / (1.f - cubeSizeFactor.get()); // [-1,1] within each scaled cell
  float distFromCorner = glm::length(1.f - abs(coordLocal));

  // logic to only allow picking nodes/cells (e.g. if no cell data is registered, only pick nodes)
  // if neither has been used, allow picking both
  bool doPickNodes;
  if (nodesHaveBeenUsed == cellsHaveBeenUsed) {
    // both or neither used (choose based on radius)
    doPickNodes = distFromCorner < nodePickRad;
  } else if (nodesHaveBeenUsed) {
    doPickNodes = true;
  } else /* cellsHaveBeenUsed == true */ {
    doPickNodes = false;
  }


  if (doPickNodes) {
    // Pick a node

    glm::uvec3 nodeInd3{std::round(coordUnit.x), std::round(coordUnit.y), std::round(coordUnit.z)};
    uint64_t nodeInd = flattenNodeIndex(nodeInd3);

    buildNodeInfoGUI(nodeInd);

  } else {
    // Pick a cell

    glm::uvec3 cellInd3{std::floor(coordUnit.x), std::floor(coordUnit.y), std::floor(coordUnit.z)};
    cellInd3 = clamp(cellInd3, glm::uvec3(0), gridCellDim - 1u);
    uint64_t cellInd = flattenCellIndex(cellInd3);

    buildCellInfoGUI(cellInd);
  }
}


void VolumeGrid::buildNodeInfoGUI(size_t nInd) {

  size_t displayInd = nInd;
  glm::uvec3 nodeInd3 = unflattenNodeIndex(nInd);

  ImGui::TextUnformatted(("Node #" + std::to_string(displayInd)).c_str());
  ImGui::TextUnformatted(("Node index: (" + std::to_string(nodeInd3.x) + "," + std::to_string(nodeInd3.y) + "," +
                          std::to_string(nodeInd3.z) + ")")
                             .c_str());


  std::stringstream buffer;
  buffer << positionOfNodeIndex(nInd);
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildNodeInfoGUI(nInd);
  }

  ImGui::Indent(-20.);
}

void VolumeGrid::buildCellInfoGUI(size_t cellInd) {

  size_t displayInd = cellInd;
  glm::uvec3 cellInd3 = unflattenCellIndex(cellInd);

  ImGui::TextUnformatted(("Cell #" + std::to_string(displayInd)).c_str());
  ImGui::TextUnformatted(("Cell index: (" + std::to_string(cellInd3.x) + "," + std::to_string(cellInd3.y) + "," +
                          std::to_string(cellInd3.z) + ")")
                             .c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildCellInfoGUI(cellInd);
  }

  ImGui::Indent(-20.);
}

VolumeGrid* registerVolumeGrid(std::string name, glm::uvec3 gridNodeDim, glm::vec3 boundMin, glm::vec3 boundMax) {
  VolumeGrid* s = new VolumeGrid(name, gridNodeDim, boundMin, boundMax);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

VolumeGrid* registerVolumeGrid(std::string name, uint64_t gridNodeDim, glm::vec3 boundMin, glm::vec3 boundMax) {
  return registerVolumeGrid(name, {gridNodeDim, gridNodeDim, gridNodeDim}, boundMin, boundMax);
}


// Default implementations
void VolumeGridQuantity::buildNodeInfoGUI(size_t vInd) {}
void VolumeGridQuantity::buildCellInfoGUI(size_t vInd) {}

} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/volume_grid.h"

#include "imgui.h"

namespace polyscope {

// Initialize statics
const std::string VolumeGrid::structureTypeName = "Volume Grid";

VolumeGrid::VolumeGrid(std::string name, glm::uvec3 gridNodeDim_, glm::vec3 boundMin_, glm::vec3 boundMax_)
    : QuantityStructure<VolumeGrid>(name, typeName()), gridNodeDim(gridNodeDim_), gridCellDim(gridNodeDim_ - 1u),
      boundMin(boundMin_), boundMax(boundMax_),

      // clang-format off
      // == managed quantities
      gridPlaneReferencePositions(uniquePrefix() +  "#gridPlaneReferencePositions",     gridPlaneReferencePositionsData,    std::bind(&VolumeGrid::computeGridPlaneReferenceGeometry, this)),
      gridPlaneReferenceNormals(uniquePrefix() +    "#gridPlaneReferenceNormals",       gridPlaneReferenceNormalsData,      [](){/* do nothing, gets handled by position func */} ),
      gridPlaneAxisInds(uniquePrefix() +            "#gridPlaneAxisInds",               gridPlaneAxisIndsData,              [](){/* do nothing, gets handled by position func */} ),

      // == persistent options
      color(                  uniquePrefix() + "color",             getNextUniqueColor()),
      edgeColor(              uniquePrefix() + "edgeColor",         glm::vec3{0., 0., 0.}), 
      material(               uniquePrefix() + "material",          "clay"),
      edgeWidth(              uniquePrefix() + "edgeWidth",         0.f),
      cubeSizeFactor(         uniquePrefix() + "cubeSizeFactor",    0.f)
// clang-format on
{
  updateObjectSpaceBounds();
}

void VolumeGrid::buildCustomUI() {
  ImGui::Text("dim (%lld, %lld, %lld)", static_cast<long long int>(gridNodeDim.x),
              static_cast<long long int>(gridNodeDim.y), static_cast<long long int>(gridNodeDim.z));

  // these all take up too much space
  // ImGui::TextUnformatted(("min: " + to_string_short(boundMin)).c_str());
  // ImGui::TextUnformatted(("max: " + to_string_short(boundMax)).c_str());
  // ImGui::TextUnformatted((to_string_short(boundMin) + " -- " + to_string_short(boundMax)).c_str());

  { // Colors
    if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(color.get());
    ImGui::SameLine();
  }


  { // Flat shading or smooth shading?
    ImGui::SameLine();
    ImGui::PushItemWidth(65);
    if (ImGui::SliderFloat("Shrink", &cubeSizeFactor.get(), 0.0, 1., "%.3f", ImGuiSliderFlags_Logarithmic)) {
      cubeSizeFactor.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();
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

    render::engine->setBackfaceCull(true);

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

  if (getEdgeWidth() > 0) {
    initRules.push_back("GRIDCUBE_WIREFRAME");
    initRules.push_back("WIREFRAME_SIMPLE");
  }

  return initRules;
}

void VolumeGrid::setGridCubeUniforms(render::ShaderProgram& p) {

  p.setUniform("u_boundMin", boundMin);
  p.setUniform("u_boundMax", boundMax);
  p.setUniform("u_cubeSizeFactor", 1.f - cubeSizeFactor.get());
  glm::vec3 refSpacing{1.f / gridCellDim.x, 1.f / gridCellDim.y, 1.f / gridCellDim.z};
  p.setUniform("u_gridSpacingReference", refSpacing);

  if (getEdgeWidth() > 0) {
    p.setUniform("u_edgeWidth", getEdgeWidth() * render::engine->getCurrentPixelScaling());
    p.setUniform("u_edgeColor", getEdgeColor());
  }

  // std::cout << "boundMin " << boundMin << std::endl;
  // std::cout << "boundMax " << boundMax << std::endl;
  // std::cout << "cubeSizeFactor " << cubeSizeFactor.get() << std::endl;
  // std::cout << "gridSpacingRef " << refSpacing << std::endl;
}

void VolumeGrid::ensureGridCubeRenderProgramPrepared() {
  // If already prepared, do nothing
  if (program) return;

  // clang-format off
  program = render::engine->requestShader(
      "GRIDCUBE_PLANE", 
      addGridCubeRules({"SHADE_BASECOLOR"})
  );
  // clang-format on

  program->setAttribute("a_referencePosition", gridPlaneReferencePositions.getRenderAttributeBuffer());
  program->setAttribute("a_referenceNormal", gridPlaneReferenceNormals.getRenderAttributeBuffer());
  program->setAttribute("a_axisInd", gridPlaneAxisInds.getRenderAttributeBuffer());

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
  objectSpaceBoundingBox = std::make_tuple(boundMin, boundMax);
  objectSpaceLengthScale = glm::length(boundMax - boundMin);
}

std::string VolumeGrid::typeName() { return structureTypeName; }

void VolumeGrid::refresh() {
  QuantityStructure<VolumeGrid>::refresh(); // call base class version, which refreshes quantities

  program.reset();
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

VolumeGrid* registerVolumeGrid(std::string name, glm::uvec3 gridNodeDim, glm::vec3 boundMin, glm::vec3 boundMax) {
  VolumeGrid* s = new VolumeGrid(name, gridNodeDim, boundMin, boundMax);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

VolumeGrid* registerVolumeGrid(std::string name, size_t gridNodeDim, glm::vec3 boundMin, glm::vec3 boundMax) {
  return registerVolumeGrid(name, {gridNodeDim, gridNodeDim, gridNodeDim}, boundMin, boundMax);
}

} // namespace polyscope

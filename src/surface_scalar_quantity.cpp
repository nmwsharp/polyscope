// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                             DataType dataType_)
    : SurfaceMeshQuantity(name, mesh_, true), dataType(dataType_),
      cMap(uniquePrefix() + name + "#cmap", defaultColorMap(dataType)), definedOn(definedOn_) {}

void SurfaceScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  parent.setStructureUniforms(*program);
  setProgramUniforms(*program);

  program->draw();
}


// Update range uniforms
void SurfaceScalarQuantity::setProgramUniforms(render::ShaderProgram& program) {
  program.setUniform("u_rangeLow", vizRange.first);
  program.setUniform("u_rangeHigh", vizRange.second);
}

SurfaceScalarQuantity* SurfaceScalarQuantity::resetMapRange() {
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
  return this;
}

void SurfaceScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    if (ImGui::MenuItem("Reset colormap range")) resetMapRange();

    ImGui::EndPopup();
  }

  if (render::buildColormapSelector(cMap.get())) {
    program.reset();
    setColorMap(getColorMap());
  }

  // Draw the histogram of values
  hist.colormapRange = vizRange;
  hist.buildUI();

  // Data range
  // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRange.first, &vizRange.second, (dataRange.second - dataRange.first) / 100.,
                             dataRange.first, dataRange.second, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
      ImGui::DragFloatRange2("##range_symmetric", &vizRange.first, &vizRange.second, absRange / 100., -absRange,
                             absRange, "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRange.first, &vizRange.second, vizRange.second / 100., 0.0,
                             dataRange.second, "Min: %.3e", "Max: %.3e");
    } break;
    }
  }
}

void SurfaceScalarQuantity::refresh() { 
  program.reset(); 
  Quantity::refresh();
}

SurfaceScalarQuantity* SurfaceScalarQuantity::setColorMap(std::string name) {
  cMap = name;
  hist.updateColormap(cMap.get());
  requestRedraw();
  return this;
}
std::string SurfaceScalarQuantity::getColorMap() { return cMap.get(); }

SurfaceScalarQuantity* SurfaceScalarQuantity::setMapRange(std::pair<double, double> val) {
  vizRange = val;
  requestRedraw();
  return this;
}
std::pair<double, double> SurfaceScalarQuantity::getMapRange() { return vizRange; }

std::string SurfaceScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

SurfaceVertexScalarQuantity::SurfaceVertexScalarQuantity(std::string name, std::vector<double> values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values, parent.vertexAreas);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void SurfaceVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}


void SurfaceVertexScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<double> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      size_t vC = face[(j + 1) % D];

      colorval.push_back(values[vRoot]);
      colorval.push_back(values[vB]);
      colorval.push_back(values[vC]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_value", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceVertexScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[vInd]);
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceFaceScalarQuantity::SurfaceFaceScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                                                     DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "face", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values, parent.faceAreas);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void SurfaceFaceScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceFaceScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<double> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();
    size_t triDegree = std::max(0, static_cast<int>(D) - 2);
    for (size_t j = 0; j < 3 * triDegree; j++) {
      colorval.push_back(values[iF]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_value", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceFaceScalarQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[fInd]);
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

SurfaceEdgeScalarQuantity::SurfaceEdgeScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                                                     DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values, parent.edgeLengths);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void SurfaceEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_HALFEDGE_VALUE", "SHADE_COLORMAP_VALUE"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceEdgeScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  // Fill buffers as usual, but at edges introduced by triangulation substitute the average value.
  // TODO this still doesn't look too great on polygon meshes... perhaps compute an average value per edge?
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // First, compute an average value for the face
    double avgVal = 0.0;
    for (size_t j = 0; j < D; j++) {
      avgVal += values[parent.edgeIndices[iF][j]];
    }
    avgVal /= D;

    // implicitly triangulate from root
    for (size_t j = 1; (j + 1) < D; j++) {

      glm::vec3 combinedValues = {avgVal, values[parent.edgeIndices[iF][j]], avgVal};

      if (j == 1) {
        combinedValues.x = values[parent.edgeIndices[iF][0]];
      }
      if (j + 2 == D) {
        combinedValues.z = values[parent.edgeIndices[iF].back()];
      }

      for (size_t i = 0; i < 3; i++) {
        colorval.push_back(combinedValues);
      }
    }
  }

  // Store data in buffers
  p.setAttribute("a_value3", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[eInd]);
  ImGui::NextColumn();
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

SurfaceHalfedgeScalarQuantity::SurfaceHalfedgeScalarQuantity(std::string name, std::vector<double> values_,
                                                             SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "halfedge", dataType_), values(std::move(values_))

{

  std::vector<double> weightsVec(parent.nHalfedges());
  size_t iHe = 0;
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();
    for (size_t j = 0; j < D; j++) {
      weightsVec[iHe] = parent.edgeLengths[parent.edgeIndices[iF][j]];
      iHe++;
    }
  }

  hist.updateColormap(cMap.get());
  hist.buildHistogram(values, weightsVec);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void SurfaceHalfedgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_HALFEDGE_VALUE", "SHADE_COLORMAP_VALUE"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceHalfedgeScalarQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  // Fill buffers as usual, but at edges introduced by triangulation substitute the average value.
  // TODO this still doesn't look too great on polygon meshes... perhaps compute an average value per edge?
  size_t iHe = 0;
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // First, compute an average value for the face
    double avgVal = 0.0;
    for (size_t j = 0; j < D; j++) {
      avgVal += values[iHe + j];
    }
    avgVal /= D;

    // implicitly triangulate from root
    for (size_t j = 1; (j + 1) < D; j++) {
      glm::vec3 combinedValues = {avgVal, avgVal, avgVal};

      if (j == 1) {
        combinedValues[0] = values[iHe];
        iHe++;
      }

      combinedValues[1] = values[iHe];
      iHe++;

      if (j + 2 == D) {
        combinedValues[2] = values[iHe];
        iHe++;
      }

      for (size_t i = 0; i < 3; i++) {
        colorval.push_back(combinedValues);
      }
    }
  }


  // Store data in buffers
  p.setAttribute("a_value3", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceHalfedgeScalarQuantity::buildHalfedgeInfoGUI(size_t heInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[heInd]);
  ImGui::NextColumn();
}

} // namespace polyscope

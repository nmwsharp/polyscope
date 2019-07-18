// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                             DataType dataType_)
    : SurfaceMeshQuantity(name, mesh_, true), dataType(dataType_), definedOn(definedOn_) {

  // Set the default colormap based on what kind of data is given
  cMap = defaultColorMap(dataType);
}

void SurfaceScalarQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  setProgramUniforms(*program);

  program->draw();
}

void SurfaceScalarQuantity::writeToFile(std::string filename) {
  polyscope::warning("Writing to file not yet implemented for this datatype");
}


// Update range uniforms
void SurfaceScalarQuantity::setProgramUniforms(gl::GLProgram& program) {
  program.setUniform("u_rangeLow", vizRangeLow);
  program.setUniform("u_rangeHigh", vizRangeHigh);
}

void SurfaceScalarQuantity::resetVizRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRangeLow = dataRangeLow;
    vizRangeHigh = dataRangeHigh;
    break;
  case DataType::SYMMETRIC: {
    float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
    vizRangeLow = -absRange;
    vizRangeHigh = absRange;
  } break;
  case DataType::MAGNITUDE:
    vizRangeLow = 0.0;
    vizRangeHigh = dataRangeHigh;
    break;
  }
}

void SurfaceScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    if (ImGui::MenuItem("Write to file")) writeToFile();
    if (ImGui::MenuItem("Reset colormap range")) resetVizRange();

    ImGui::EndPopup();
  }

  if (buildColormapSelector(cMap)) {
    program.reset();
    hist.updateColormap(cMap);
  }

  // Draw the histogram of values
  hist.colormapRangeMin = vizRangeLow;
  hist.colormapRangeMax = vizRangeHigh;
  hist.buildUI();

  // Data range
  // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRangeLow, &vizRangeHigh, (dataRangeHigh - dataRangeLow) / 100., dataRangeLow,
                             dataRangeHigh, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
      ImGui::DragFloatRange2("##range_symmetric", &vizRangeLow, &vizRangeHigh, absRange / 100., -absRange, absRange,
                             "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRangeLow, &vizRangeHigh, vizRangeHigh / 100., 0.0, dataRangeHigh,
                             "Min: %.3e", "Max: %.3e");
    } break;
    }
  }
}

void SurfaceScalarQuantity::geometryChanged() { program.reset(); }

std::string SurfaceScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

SurfaceVertexScalarQuantity::SurfaceVertexScalarQuantity(std::string name, std::vector<double> values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap);
  hist.buildHistogram(values, parent.vertexAreas);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void SurfaceVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(new gl::GLProgram(&gl::VERTCOLOR_SURFACE_VERT_SHADER, &gl::VERTCOLOR_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}


void SurfaceVertexScalarQuantity::fillColorBuffers(gl::GLProgram& p) {
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
  p.setAttribute("a_colorval", colorval);
  p.setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
}

void SurfaceVertexScalarQuantity::writeToFile(std::string filename) {

  throw std::runtime_error("not implemented");

  /* TODO
  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  // For now, just always write scalar to U texture coordinate

  cout << "Writing vertex value to file " << filename << " in U coordinate of texture map" << endl;

  HalfedgeMesh* mesh = parent.mesh;
  CornerData<Vector2> scalarVal(mesh, Vector2{0.0, 0.0});
  for (CornerPtr c : mesh->corners()) {
    scalarVal[c].x = values[c.vertex()];
  }

  WavefrontOBJ::write(filename, *parent.geometry, scalarVal);
  */
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
  hist.updateColormap(cMap);
  hist.buildHistogram(values, parent.faceAreas);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void SurfaceFaceScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(new gl::GLProgram(&gl::VERTCOLOR_SURFACE_VERT_SHADER, &gl::VERTCOLOR_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}

void SurfaceFaceScalarQuantity::fillColorBuffers(gl::GLProgram& p) {
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
  p.setAttribute("a_colorval", colorval);
  p.setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
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
  hist.updateColormap(cMap);
  hist.buildHistogram(values, parent.edgeLengths);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void SurfaceEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(new gl::GLProgram(&gl::HALFEDGECOLOR_SURFACE_VERT_SHADER, &gl::HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}

void SurfaceEdgeScalarQuantity::fillColorBuffers(gl::GLProgram& p) {
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
  p.setAttribute("a_colorvals", colorval);
  p.setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
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

  hist.updateColormap(cMap);
  hist.buildHistogram(values, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void SurfaceHalfedgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(new gl::GLProgram(&gl::HALFEDGECOLOR_SURFACE_VERT_SHADER, &gl::HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}

void SurfaceHalfedgeScalarQuantity::fillColorBuffers(gl::GLProgram& p) {
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
  p.setAttribute("a_colorvals", colorval);
  p.setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
}

void SurfaceHalfedgeScalarQuantity::buildHalfedgeInfoGUI(size_t heInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[heInd]);
  ImGui::NextColumn();
}

} // namespace polyscope

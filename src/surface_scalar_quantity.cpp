#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn_,
                                             DataType dataType_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), dataType(dataType_), definedOn(definedOn_) {
  // Set the default colormap based on what kind of data is given
  switch (dataType) {
  case DataType::STANDARD:
    iColorMap = 0; // viridis
    break;
  case DataType::SYMMETRIC:
    iColorMap = 1; // coolwarm
    break;
  case DataType::MAGNITUDE:
    iColorMap = 2; // blues
    break;
  }
}

void SurfaceScalarQuantity::draw() {}


// Update range uniforms
void SurfaceScalarQuantity::setProgramValues(gl::GLProgram* program) {
  program->setUniform("u_rangeLow", vizRangeLow);
  program->setUniform("u_rangeHigh", vizRangeHigh);
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

void SurfaceScalarQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (" + definedOn + " scalar)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    { // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColormapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, cm_names, IM_ARRAYSIZE(cm_names));
      ImGui::PopItemWidth();
      if (iColorMap != iColormapBefore) {
        parent->deleteProgram();
        hist.updateColormap(colormaps[iColorMap]);
      }
    }

    // Reset button
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
      resetVizRange();
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

    ImGui::TreePop();
  }

  // Enforce exclusivity of enabled surface quantities
  if (!enabledBefore && enabled) {
    parent->setActiveSurfaceQuantity(this);
  }
  if (enabledBefore && !enabled) {
    parent->clearActiveSurfaceQuantity();
  }
}

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

SurfaceScalarVertexQuantity::SurfaceScalarVertexQuantity(std::string name, VertexData<double>& values_,
                                                         SurfaceMesh* mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (VertexPtr v : parent->mesh->vertices()) {
    valsVec.push_back(values[v]);
    weightsVec.push_back(parent->geometry->dualArea(v));
  }

  hist.updateColormap(colormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceScalarVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER, &VERTCOLOR_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}


void SurfaceScalarVertexQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<double> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      double c2 = values[v];
      if (iP >= 2) {
        colorval.push_back(c0);
        colorval.push_back(c1);
        colorval.push_back(c2);
      }
      c0 = c1;
      c1 = c2;
      iP++;
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
  p->setTextureFromColormap("t_colormap", *colormaps[iColorMap]);
}

void SurfaceScalarVertexQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[v]);
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceScalarFaceQuantity::SurfaceScalarFaceQuantity(std::string name, FaceData<double>& values_, SurfaceMesh* mesh_,
                                                     DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "face", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (FacePtr f : parent->mesh->faces()) {
    valsVec.push_back(values[f]);
    weightsVec.push_back(parent->geometry->area(f));
  }

  hist.updateColormap(colormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceScalarFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER, &VERTCOLOR_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceScalarFaceQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<double> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      double c2 = values[f];
      if (iP >= 2) {
        colorval.push_back(c0);
        colorval.push_back(c1);
        colorval.push_back(c2);
      }
      c0 = c1;
      c1 = c2;
      iP++;
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
  p->setTextureFromColormap("t_colormap", *colormaps[iColorMap]);
}

void SurfaceScalarFaceQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[f]);
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

SurfaceScalarEdgeQuantity::SurfaceScalarEdgeQuantity(std::string name, EdgeData<double>& values_, SurfaceMesh* mesh_,
                                                     DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (EdgePtr e : parent->mesh->edges()) {
    valsVec.push_back(values[e]);
    double area = parent->geometry->area(e.halfedge().face()) / 3.0;
    if (!e.isBoundary()) {
      area += parent->geometry->area(e.halfedge().twin().face()) / 3.0;
    }
    weightsVec.push_back(area);
  }

  hist.updateColormap(colormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceScalarEdgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER, &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceScalarEdgeQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Vector3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {
      VertexPtr v = he.vertex();
      double c2 = values[he.next().edge()];
      if (iP >= 2) {
        colorval.push_back(Vector3{c0, c1, c2});
        colorval.push_back(Vector3{c0, c1, c2});
        colorval.push_back(Vector3{c0, c1, c2});
      }
      if (iP > 2) {
        error("Edge quantities not correct for non-triangular meshes");
      }
      c0 = c1;
      c1 = c2;
      iP++;
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorvals", colorval);
  p->setTextureFromColormap("t_colormap", *colormaps[iColorMap]);
}

void SurfaceScalarEdgeQuantity::buildInfoGUI(EdgePtr e) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[e]);
  ImGui::NextColumn();
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

SurfaceScalarHalfedgeQuantity::SurfaceScalarHalfedgeQuantity(std::string name, HalfedgeData<double>& values_,
                                                             SurfaceMesh* mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "halfedge", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (HalfedgePtr he : parent->mesh->halfedges()) {
    valsVec.push_back(values[he]);
    double area = 0;
    if (he.isReal()) {
      area += parent->geometry->area(he.face()) / 3.0;
    }
  }

  hist.updateColormap(colormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceScalarHalfedgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER, &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceScalarHalfedgeQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Vector3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {
      VertexPtr v = he.vertex();
      double c2 = values[he.next()];
      if (iP >= 2) {
        colorval.push_back(Vector3{c0, c1, c2});
        colorval.push_back(Vector3{c0, c1, c2});
        colorval.push_back(Vector3{c0, c1, c2});
      }
      if (iP > 2) {
        error("Edge quantities not correct for non-triangular meshes");
      }
      c0 = c1;
      c1 = c2;
      iP++;
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorvals", colorval);
  p->setTextureFromColormap("t_colormap", *colormaps[iColorMap]);
}

void SurfaceScalarHalfedgeQuantity::buildInfoGUI(HalfedgePtr he) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[he]);
  ImGui::NextColumn();
}

} // namespace polyscope

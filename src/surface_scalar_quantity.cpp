#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name,
                                             SurfaceMesh* mesh_,
                                             std::string definedOn_,
                                             DataType dataType_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), dataType(dataType_), definedOn(definedOn_){}

double SurfaceScalarQuantity::mapVal(double x) {
  return geometrycentral::clamp((x - minVal) / (maxVal - minVal), 0.0, 1.0);
}

std::string SurfaceScalarQuantity::dataBoundsString() {
  size_t bSize = 50;
  char b[bSize];
  snprintf(b, bSize, "[%6.2e, %6.2e]", minVal, maxVal);
  return std::string(b);
}

void SurfaceScalarQuantity::draw() {
}  // nothing to do, drawn by surface mesh program

void SurfaceScalarQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (" + definedOn + " scalar)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    {  // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColormapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, cm_names, IM_ARRAYSIZE(cm_names));
      if (iColorMap != iColormapBefore && enabled) {
        parent->deleteProgram();
      }
    }

    { // Draw max and min
      ImGui::TextUnformatted(dataBoundsString().c_str());
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

SurfaceScalarVertexQuantity::SurfaceScalarVertexQuantity(
    std::string name, VertexData<double>& values_, SurfaceMesh* mesh_,
    DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", dataType_)

{
  values = parent->transfer.transfer(values_);

  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
  for (VertexPtr v : parent->mesh->vertices()) {
    minVal = std::min(minVal, values[v]);
    maxVal = std::max(maxVal, values[v]);
  }

  // Hack to do less ugly things when constants are passed in
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }
}

gl::GLProgram* SurfaceScalarVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER,
                                             &VERTCOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

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
      double c2 = mapVal(values[v]);
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

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceScalarFaceQuantity::SurfaceScalarFaceQuantity(
    std::string name, FaceData<double>& values_, SurfaceMesh* mesh_,
    DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "face", dataType_)

{
  values = parent->transfer.transfer(values_);

  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
  for (FacePtr f : parent->mesh->faces()) {
    minVal = std::min(minVal, values[f]);
    maxVal = std::max(maxVal, values[f]);
  }

  // Hack to do less ugly things when constants are passed in
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }
}

gl::GLProgram* SurfaceScalarFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER,
                                             &VERTCOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

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
      double c2 = mapVal(values[f]);
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


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

SurfaceScalarEdgeQuantity::SurfaceScalarEdgeQuantity(
    std::string name, EdgeData<double>& values_, SurfaceMesh* mesh_,
    DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", dataType_)

{
  values = parent->transfer.transfer(values_);

  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
  for (EdgePtr e : parent->mesh->edges()) {
    minVal = std::min(minVal, values[e]);
    maxVal = std::max(maxVal, values[e]);
  }

  // Hack to do less ugly things when constants are passed in
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }
}

gl::GLProgram* SurfaceScalarEdgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER,
                                             &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
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
      double c2 = mapVal(values[he.next().edge()]);
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

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

SurfaceScalarHalfedgeQuantity::SurfaceScalarHalfedgeQuantity(
    std::string name, HalfedgeData<double>& values_, SurfaceMesh* mesh_,
    DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "halfedge", dataType_)

{
  values = parent->transfer.transfer(values_);

  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
  for (HalfedgePtr he : parent->mesh->halfedges()) {
    minVal = std::min(minVal, values[he]);
    maxVal = std::max(maxVal, values[he]);
  }

  // Hack to do less ugly things when constants are passed in
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }
}

gl::GLProgram* SurfaceScalarHalfedgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER,
                                             &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
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
      double c2 = mapVal(values[he.next()]);
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

}  // namespace polyscope
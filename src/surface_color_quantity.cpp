#include "polyscope/surface_color_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceColorQuantity::SurfaceColorQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), definedOn(definedOn_) {}

void SurfaceColorQuantity::draw() {} // nothing to do, drawn by surface mesh program

void SurfaceColorQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (" + definedOn + " Color)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

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
// ==========           Vertex Color            ==========
// ========================================================

SurfaceColorVertexQuantity::SurfaceColorVertexQuantity(std::string name, VertexData<Vector3>& values_,
                                                       SurfaceMesh* mesh_)
    : SurfaceColorQuantity(name, mesh_, "vertex")

{
  values = parent->transfer.transfer(values_);
}

gl::GLProgram* SurfaceColorVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceColorVertexQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Vector3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    Vector3 c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 c2 = values[v];
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
}

void SurfaceColorVertexQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::array<float, 3> tempColor = values[v].toFloatArray();
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[v];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}


// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceColorFaceQuantity::SurfaceColorFaceQuantity(std::string name, FaceData<Vector3>& values_, SurfaceMesh* mesh_)
    : SurfaceColorQuantity(name, mesh_, "face")

{
  values = parent->transfer.transfer(values_);
}

gl::GLProgram* SurfaceColorFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceColorFaceQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Vector3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    Vector3 c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 c2 = values[f];
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
}

void SurfaceColorFaceQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::array<float, 3> tempColor = values[f].toFloatArray();
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[f];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

/*
// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

SurfaceScalarEdgeQuantity::SurfaceScalarEdgeQuantity(std::string name,
                                                     EdgeData<double>& values_,
                                                     SurfaceMesh* mesh_,
                                                     DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  for (EdgePtr e : parent->mesh->edges()) {
    valsVec.push_back(values[e]);
  }
  mapper = AffineRemapper<double>(valsVec, dataType);
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
      double c2 = mapper.map(values[he.next().edge()]);
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

  std::vector<double> valsVec;
  for (HalfedgePtr he : parent->mesh->halfedges()) {
    valsVec.push_back(values[he]);
  }
  mapper = AffineRemapper<double>(valsVec, dataType);
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
      double c2 = mapper.map(values[he.next()]);
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
*/

} // namespace polyscope
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
  if (ImGui::TreeNode((name + " (" + definedOn + " Color)").c_str())) {

    bool currEnabled = enabled;
    if (ImGui::Checkbox("Enabled", &currEnabled)) {
      setEnabled(currEnabled);
    }

    ImGui::TreePop();
  }
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

SurfaceColorVertexQuantity::SurfaceColorVertexQuantity(std::string name, std::vector<Color3f> values_,
                                                       SurfaceMesh* mesh_)
    : SurfaceColorQuantity(name, mesh_, "vertex"), values(std::move(values_))

{}

gl::GLProgram* SurfaceColorVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceColorVertexQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Color3f> colorval;
  colorval.resize(3 * parent->triMesh.nFaces());

  for (const HalfedgeMesh::Face& face : parent->triMesh.faces) {
    for(size_t i = 0; i < 3; i++) {
      size_t vInd = face.triangleVertices()[i]->index();
      colorval.push_back(values[vInd]);
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
}

void SurfaceColorVertexQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  Color3f tempColor = values[vInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}


// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceColorFaceQuantity::SurfaceColorFaceQuantity(std::string name, std::vector<Color3f> values_, SurfaceMesh* mesh_)
    : SurfaceColorQuantity(name, mesh_, "face"), values(std::move(values_))

{}

gl::GLProgram* SurfaceColorFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceColorFaceQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<Color3f> colorval;
  colorval.resize(3 * parent->triMesh.nFaces());

  for (const HalfedgeMesh::Face& face : parent->triMesh.faces) {
    size_t fInd = face.index();
    for (size_t i = 0; i < 3; i++) {
      colorval.push_back(values[fInd]);
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
}

void SurfaceColorFaceQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  Color3f tempColor = values[fInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[fInd];
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
  std::vector<glm::vec3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {
      VertexPtr v = he.vertex();
      double c2 = mapper.map(values[he.next().edge()]);
      if (iP >= 2) {
        colorval.push_back(glm::vec3{c0, c1, c2});
        colorval.push_back(glm::vec3{c0, c1, c2});
        colorval.push_back(glm::vec3{c0, c1, c2});
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
  std::vector<glm::vec3> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {
      VertexPtr v = he.vertex();
      double c2 = mapper.map(values[he.next()]);
      if (iP >= 2) {
        colorval.push_back(glm::vec3{c0, c1, c2});
        colorval.push_back(glm::vec3{c0, c1, c2});
        colorval.push_back(glm::vec3{c0, c1, c2});
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

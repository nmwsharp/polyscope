#include "polyscope/surface_selection_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

using std::cout;
using std::endl;

SurfaceSelectionQuantity::SurfaceSelectionQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), definedOn(definedOn_) {
  iColorMap = gl::getColormapIndex_all("blues");
}

void SurfaceSelectionQuantity::draw() {}


void SurfaceSelectionQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (" + definedOn + " selection)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    { // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColormapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, gl::allColormapNames, IM_ARRAYSIZE(gl::allColormapNames));
      ImGui::PopItemWidth();
      if (iColorMap != iColormapBefore) {
        parent->deleteProgram();
      }
    }

    if(allowEditingFromDefaultUI) {
      if (ImGui::Button("Edit")) {
        userEdit();
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
// ==========           Vertex Selection         ==========
// ========================================================

SurfaceSelectionVertexQuantity::SurfaceSelectionVertexQuantity(std::string name, VertexData<char>& membership_,
                                                               SurfaceMesh* mesh_)
    : SurfaceSelectionQuantity(name, mesh_, "vertex")

{
  membership = parent->transfer.transfer(membership_);
}

gl::GLProgram* SurfaceSelectionVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTBINARY_SURFACE_VERT_SHADER, &VERTBINARY_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}


void SurfaceSelectionVertexQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<double> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      double c2 = membership[v];
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
  membershipStale = false;

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
  p->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap]);
}

void SurfaceSelectionVertexQuantity::setProgramValues(gl::GLProgram* program) {

  // Update membership buffer, if needed
  if (membershipStale) {

    std::vector<double> colorval;
    colorval.reserve(parent->mesh->nHalfedges());
    for (FacePtr f : parent->mesh->faces()) {
      // Implicitly triangulate
      double c0, c1;
      size_t iP = 0;
      for (VertexPtr v : f.adjacentVertices()) {
        double c2 = membership[v];
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
    program->setAttribute("a_colorval", colorval, true); // update buffer


    membershipStale = false;
  }
}

void SurfaceSelectionVertexQuantity::userEdit() {

  // Make sure we can see what we're editing
  enabled = true;
  parent->setActiveSurfaceQuantity(this);

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();
  bool oldAlwaysPick = pick::alwaysEvaluatePick;
  pick::alwaysEvaluatePick = true;

  // Register the callback which creates the UI and does the hard work
  focusedPopupUI = std::bind(&SurfaceSelectionVertexQuantity::userEditCallback, this);

  // Re-enter main loop
  while (focusedPopupUI) {
    mainLoopIteration();
  }

  // Restore the old context
  pick::alwaysEvaluatePick = oldAlwaysPick;
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);
}

void SurfaceSelectionVertexQuantity::userEditCallback() {

  static bool showWindow = true;
  ImGui::Begin("Edit Vertex Selection", &showWindow);

  // Toggle what mouse does
  ImGui::Combo("with click", &mouseMemberAction, "add\0subtract\0\0");

  // Process mouse selection if the ctrl key is held, the mouse is pressed, and the mouse isn't on the ImGui window
  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl && !io.WantCaptureMouse && ImGui::IsMouseDown(0)) {
    // ImVec2 p = ImGui::GetMousePos();
    // io.DisplayFramebufferScale.x * p.x, io.DisplayFramebufferScale.y * p.y);

    // Check if the pick landed on a mesh element
    size_t localInd;
    Structure* pickedStruc = pick::getCurrentPickElement(localInd);
    if (pickedStruc == parent) {

      // Find the nearest vertex (in screen space) to the selected element
      std::vector<VertexPtr> candidateVertices;
      VertexPtr vOut;
      FacePtr fOut;
      EdgePtr eOut;
      HalfedgePtr heOut;
      parent->getPickedElement(localInd, vOut, fOut, eOut, heOut);

      if (vOut != VertexPtr()) {
        candidateVertices.push_back(vOut);
      }
      if (fOut != FacePtr()) {
        for (VertexPtr v : fOut.adjacentVertices()) {
          candidateVertices.push_back(v);
        }
      }
      if (eOut != EdgePtr()) {
        candidateVertices.push_back(eOut.halfedge().vertex());
        candidateVertices.push_back(eOut.halfedge().twin().vertex());
      }
      if (heOut != HalfedgePtr()) {
        candidateVertices.push_back(heOut.vertex());
        candidateVertices.push_back(heOut.twin().vertex());
      }

      for (VertexPtr v : candidateVertices) {
        membership[v] = mouseMemberAction == 0;
        membershipStale = true;
      }
    }
  }


  // Grow the selection
  if (ImGui::Button("Grow selection")) {
    VertexData<char> newMembership(parent->mesh, false);
    for (VertexPtr v : parent->mesh->vertices()) {
      if (membership[v]) {
        newMembership[v] = true;
        for (VertexPtr n : v.adjacentVertices()) {
          newMembership[n] = true;
        }
      }
    }
    membership = newMembership;
    membershipStale = true;
  }


  // Shrink the selection
  if (ImGui::Button("Shrink selection")) {
    VertexData<char> newMembership(parent->mesh, true);
    for (VertexPtr v : parent->mesh->vertices()) {
      if (!membership[v]) {
        newMembership[v] = false;
        for (VertexPtr n : v.adjacentVertices()) {
          newMembership[n] = false;
        }
      }
    }
    membership = newMembership;
    membershipStale = true;
  }

  // Select all
  if (ImGui::Button("Select all")) {
    for (VertexPtr v : parent->mesh->vertices()) {
      membership[v] = true;
    }
    membershipStale = true;
  }

  // Select none
  ImGui::SameLine();
  if (ImGui::Button("Select none")) {
    for (VertexPtr v : parent->mesh->vertices()) {
      membership[v] = false;
    }
    membershipStale = true;
  }


  // Stop editing
  // (style makes yellow button)
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1. / 7.0f, 0.6f, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1. / 7.0f, 0.7f, 0.7f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1. / 7.0f, 0.8f, 0.8f));
  if (ImGui::Button("Done")) {
    focusedPopupUI = nullptr;
    membershipStale = true;
  }
  ImGui::PopStyleColor(3);

  ImGui::End();
}

void SurfaceSelectionVertexQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%s", membership[v] ? "true" : "false");
  ImGui::NextColumn();
}
  
VertexData<char> SurfaceSelectionVertexQuantity::getSelectionOnInputMesh() {
  return parent->transfer.transferBack(membership);
}

/*
// ========================================================
// ==========            Face Selection          ==========
// ========================================================

SurfaceSelectionFaceQuantity::SurfaceSelectionFaceQuantity(std::string name, FaceData<double>& values_, SurfaceMesh*
mesh_,
                                                     DataType dataType_)
    : SurfaceSelectionQuantity(name, mesh_, "face", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (FacePtr f : parent->mesh->faces()) {
    valsVec.push_back(values[f]);
    weightsVec.push_back(parent->geometry->area(f));
  }

  hist.updateColormap(gl::allColormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceSelectionFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER, &VERTCOLOR_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceSelectionFaceQuantity::fillColorBuffers(gl::GLProgram* p) {
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
  p->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap]);
}

void SurfaceSelectionFaceQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[f]);
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Selection          ==========
// ========================================================

SurfaceSelectionEdgeQuantity::SurfaceSelectionEdgeQuantity(std::string name, EdgeData<double>& values_, SurfaceMesh*
mesh_,
                                                     DataType dataType_)
    : SurfaceSelectionQuantity(name, mesh_, "edge", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (EdgePtr e : parent->mesh->edges()) {
    valsVec.push_back(values[e]);
    weightsVec.push_back(parent->geometry->length(e));
  }

  hist.updateColormap(gl::allColormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceSelectionEdgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER, &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceSelectionEdgeQuantity::fillColorBuffers(gl::GLProgram* p) {
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
  p->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap]);
}

void SurfaceSelectionEdgeQuantity::buildInfoGUI(EdgePtr e) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[e]);
  ImGui::NextColumn();
}

// ========================================================
// ==========          Halfedge Selection        ==========
// ========================================================

SurfaceSelectionHalfedgeQuantity::SurfaceSelectionHalfedgeQuantity(std::string name, HalfedgeData<double>& values_,
                                                             SurfaceMesh* mesh_, DataType dataType_)
    : SurfaceSelectionQuantity(name, mesh_, "halfedge", dataType_)

{
  values = parent->transfer.transfer(values_);

  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for (HalfedgePtr he : parent->mesh->halfedges()) {
    valsVec.push_back(values[he]);
    weightsVec.push_back(parent->geometry->length(he.edge()));
  }

  hist.updateColormap(gl::allColormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceSelectionHalfedgeQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&HALFEDGECOLOR_SURFACE_VERT_SHADER, &HALFEDGECOLOR_SURFACE_FRAG_SHADER,
                                             gl::DrawMode::Triangles);

￼
Prof. Ian H. Jermyn (Durham University, UK)

￼
Prof. Jonathan Manton (University of Melbourne, Australia)

￼
Prof. Stefano Soatto (UCLA)

￼

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceSelectionHalfedgeQuantity::fillColorBuffers(gl::GLProgram* p) {
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
  p->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap]);
}

void SurfaceSelectionHalfedgeQuantity::buildInfoGUI(HalfedgePtr he) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[he]);
  ImGui::NextColumn();
}

*/

} // namespace polyscope

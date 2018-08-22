#include "polyscope/surface_mesh.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/gl/colors.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_count_quantity.h"

#include "stb_image.h"

#include "imgui.h"

#include <unordered_map>
#include <utility>

using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string SurfaceMesh::structureTypeName = "Surface Mesh";


SurfaceMesh::~SurfaceMesh() { deleteProgram(); }

void SurfaceMesh::deleteProgram() {
  if (program != nullptr) {
    delete program;
    program = nullptr;
  }
}

void SurfaceMesh::draw() {
  if (!enabled) {
    return;
  }

  if (program == nullptr) {
    prepare();
  }

  // Set uniforms
  glm::mat4 viewMat = getModelView();
  program->setUniform("u_modelView", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  program->setUniform("u_basecolor", surfaceColor);
  program->setUniform("u_edgeWidth", edgeWidth);

  // If the current program came from a quantity, allow the quantity to do any necessary per-frame work (like setting
  // uniforms)
  if (activeSurfaceQuantity != nullptr) {
    activeSurfaceQuantity->setProgramValues(program);
  }

  program->draw();

  // Draw the quantities
  for (auto x : quantities) {
    x.second->draw();
  }
}

void SurfaceMesh::drawPick() {
  if (!enabled) {
    return;
  }

  // Set uniforms
  glm::mat4 viewMat = getModelView();
  pickProgram->setUniform("u_modelView", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  pickProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  pickProgram->draw();
}

void SurfaceMesh::prepare() {
  // It not quantity is coloring the surface, draw with a default color
  if (activeSurfaceQuantity == nullptr) {
    program = new gl::GLProgram(&PLAIN_SURFACE_VERT_SHADER, &PLAIN_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);
  }
  // If some quantity is responsible for coloring the surface, prepare it
  else {
    program = activeSurfaceQuantity->createProgram();
  }

  setMaterialForProgram(program, "wax");

  // Populate draw buffers
  fillGeometryBuffers();
}

void SurfaceMesh::preparePick() {

  // Create a new program
  pickProgram = new gl::GLProgram(&PICK_SURFACE_VERT_SHADER, &PICK_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Get element indices
  size_t totalPickElements =
      triMesh.nVertices() + triMesh.nOrigFaces() + triMesh.nOrigEdges() + triMesh.nOrigHalfedges();

  // In "local" indices, indexing elements only within this triMesh, used for reading later
  facePickIndStart = triMesh.nVertices();
  edgePickIndStart = facePickIndStart + triMesh.nOrigFaces();
  halfedgePickIndStart = edgePickIndStart + triMesh.nOrigEdges();

  // In "global" indices, indexing all elements in the scene, used to fill buffers for drawing here
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);
  size_t faceGlobalPickIndStart = pickStart + triMesh.nVertices();
  size_t edgeGlobalPickIndStart = faceGlobalPickIndStart + triMesh.nOrigFaces();
  size_t halfedgeGlobalPickIndStart = edgeGlobalPickIndStart + triMesh.nOrigEdges();

  // == Fill buffers
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> bcoord;
  std::vector<std::array<glm::vec3, 3>> vertexColors, edgeColors, halfedgeColors;
  std::vector<glm::vec3> faceColor;

  // Reserve space
  positions.reserve(3 * triMesh.nFaces());
  bcoord.reserve(3 * triMesh.nFaces());
  vertexColors.reserve(3 * triMesh.nFaces());
  edgeColors.reserve(3 * triMesh.nFaces());
  halfedgeColors.reserve(3 * triMesh.nFaces());
  faceColor.reserve(3 * triMesh.nFaces());

  // Loop through triangulation to fill buffers
  for (HalfedgeMesh::Face& face : triMesh.faces) {

    glm::vec3 fColor = pick::indToVec(face.index() + faceGlobalPickIndStart);

    // Build all quantities
    std::array<glm::vec3, 3> vColor, eColor, heColor;
    HalfedgeMesh::Halfedge* currHe = &face.halfedge();
    for (size_t i = 0; i < 3; i++) {
      size_t heInd = currHe->index();
      size_t vInd = currHe->vertex().index();
      size_t eInd = currHe->edge().index();

      // Want just one copy of positions and face color, so we can build it in the usual way
      positions.push_back(currHe->vertex().position());
      faceColor.push_back(fColor);

      // Vertex index color
      vColor[i] = pick::indToVec(vInd + pickStart);

      // Edge index color
      if (currHe->edge().hasValidIndex()) {
        eColor[i] = pick::indToVec(eInd + edgeGlobalPickIndStart);
      } else {
        // If this is a fake edge induced by triangulation, use the face's color instead
        eColor[i] = fColor;
      }

      // Halfedge index color
      if (currHe->hasValidIndex()) {
        heColor[i] = pick::indToVec(heInd + halfedgeGlobalPickIndStart);
      } else {
        // If this is a fake edge induced by triangulation, use the face's color instead
        heColor[i] = fColor;
      }

      currHe = &currHe->next();
    }

    // Push three copies of the values needed at each vertex
    for (int j = 0; j < 3; j++) {
      vertexColors.push_back(vColor);
      edgeColors.push_back(eColor);
      halfedgeColors.push_back(heColor);
    }

    // Just one copy of barycoords needed
    bcoord.push_back(glm::vec3{1.0, 0.0, 0.0});
    bcoord.push_back(glm::vec3{0.0, 1.0, 0.0});
    bcoord.push_back(glm::vec3{0.0, 0.0, 1.0});
  }

  // Store data in buffers
  pickProgram->setAttribute("a_position", positions);
  pickProgram->setAttribute("a_barycoord", bcoord);
  pickProgram->setAttribute<glm::vec3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_edgeColors", edgeColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_halfedgeColors", halfedgeColors);
  pickProgram->setAttribute("a_faceColor", faceColor);
}

void SurfaceMesh::fillGeometryBuffers() {
  if (shadeStyle == ShadeStyle::SMOOTH) {
    fillGeometryBuffersSmooth();
  } else if (shadeStyle == ShadeStyle::FLAT) {
    fillGeometryBuffersFlat();
  }
}

void SurfaceMesh::fillGeometryBuffersSmooth() {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;

  // Reserve space
  positions.reserve(3 * triMesh.nFaces());
  normals.reserve(3 * triMesh.nFaces());
  bcoord.reserve(3 * triMesh.nFaces());

  for (HalfedgeMesh::Face& face : triMesh.faces) {

    HalfedgeMesh::Halfedge* currHe = &face.halfedge();
    for (size_t i = 0; i < 3; i++) {

      glm::vec3 vertexPos = currHe->vertex().position();
      glm::vec3 vertexNormal = currHe->vertex().normal();
      glm::vec3 coord{0., 0., 0.};
      coord[i] = 1.0;

      positions.push_back(vertexPos);
      normals.push_back(vertexNormal);
      bcoord.push_back(coord);

      currHe = &currHe->next();
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
}

void SurfaceMesh::fillGeometryBuffersFlat() {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;

  // Reserve space
  positions.reserve(3 * triMesh.nFaces());
  normals.reserve(3 * triMesh.nFaces());
  bcoord.reserve(3 * triMesh.nFaces());

  for (HalfedgeMesh::Face& face : triMesh.faces) {
    glm::vec3 faceNormal = face.normal();

    HalfedgeMesh::Halfedge* currHe = &face.halfedge();
    for (size_t i = 0; i < 3; i++) {

      glm::vec3 vertexPos = currHe->vertex().position();
      glm::vec3 coord{0., 0., 0.};
      coord[i] = 1.0;

      positions.push_back(vertexPos);
      normals.push_back(faceNormal);
      bcoord.push_back(coord);

      currHe = &currHe->next();
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
}

void SurfaceMesh::drawSharedStructureUI() {}


void SurfaceMesh::drawPickUI(size_t localPickID) {

  // Selection type
  if (localPickID < facePickIndStart) {
    buildVertexInfoGui(localPickID);
  } else if (localPickID < edgePickIndStart) {
    buildFaceInfoGui(localPickID - facePickIndStart);
  } else if (localPickID < halfedgePickIndStart) {
    buildEdgeInfoGui(localPickID - edgePickIndStart);
  } else {
    buildHalfedgeInfoGui(localPickID - halfedgePickIndStart);
  }
}

// void SurfaceMesh::getPickedElement(size_t localPickID, VertexPtr& vOut, FacePtr& fOut, EdgePtr& eOut,
// HalfedgePtr& heOut) {

// vOut = VertexPtr();
// fOut = FacePtr();
// eOut = EdgePtr();
// heOut = HalfedgePtr();

// if (localPickID < facePickIndStart) {
// vOut = mesh->vertex(localPickID);
//} else if (localPickID < edgePickIndStart) {
// fOut = mesh->face(localPickID - facePickIndStart);
//} else if (localPickID < halfedgePickIndStart) {
// eOut = mesh->edge(localPickID - edgePickIndStart);
//} else {
// heOut = mesh->allHalfedge(localPickID - halfedgePickIndStart);
//}
//}


glm::vec2 SurfaceMesh::projectToScreenSpace(glm::vec3 coord) {

  glm::mat4 viewMat = getModelView();
  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  glm::vec4 coord4(coord.x, coord.y, coord.z, 1.0);
  glm::vec4 screenPoint = projMat * viewMat * coord4;

  return glm::vec2{screenPoint.x, screenPoint.y} / screenPoint.w;
}

// TODO fix up all of these
// bool SurfaceMesh::screenSpaceTriangleTest(size_t fInd, glm::vec2 testCoords, glm::vec3& bCoordOut) {

//// Get points in screen space
// glm::vec2 p0 = projectToScreenSpace(geometry->position(f.halfedge().vertex()));
// glm::vec2 p1 = projectToScreenSpace(geometry->position(f.halfedge().next().vertex()));
// glm::vec2 p2 = projectToScreenSpace(geometry->position(f.halfedge().next().next().vertex()));

//// Make sure triangle is positively oriented
// if (glm::cross(p1 - p0, p2 - p0).z < 0) {
// cout << "triangle not positively oriented" << endl;
// return false;
//}

//// Test the point
// glm::vec2 v0 = p1 - p0;
// glm::vec2 v1 = p2 - p0;
// glm::vec2 vT = testCoords - p0;

// double dot00 = dot(v0, v0);
// double dot01 = dot(v0, v1);
// double dot0T = dot(v0, vT);
// double dot11 = dot(v1, v1);
// double dot1T = dot(v1, vT);

// double denom = 1.0 / (dot00 * dot11 - dot01 * dot01);
// double v = (dot11 * dot0T - dot01 * dot1T) * denom;
// double w = (dot00 * dot1T - dot01 * dot0T) * denom;

//// Check if point is in triangle
// bool inTri = (v >= 0) && (w >= 0) && (v + w < 1);
// if (!inTri) {
// return false;
//}

// bCoordOut = glm::vec3{1.0 - v - w, v, w};
// return true;
//}

/*
void SurfaceMesh::getPickedFacePoint(FacePtr& fOut, glm::vec3& baryCoordOut) {

  // Get the most recent pick data
  size_t localInd;
  Structure* pickStruct = pick::getCurrentPickElement(localInd);
  if (pickStruct != this) {
    fOut = FacePtr();
    return;
  }

  // Build a list of all faces we might need to check
  std::vector<FacePtr> facesToCheck;
  if (localInd < facePickIndStart) {
    VertexPtr v = mesh->vertex(localInd);
    for (FacePtr f : v.adjacentFaces()) {
      facesToCheck.push_back(f);
    }
  } else if (localInd < edgePickIndStart) {
    FacePtr f = mesh->face(localInd - facePickIndStart);
    facesToCheck.push_back(f);
  } else if (localInd < halfedgePickIndStart) {
    EdgePtr e = mesh->edge(localInd - edgePickIndStart);
    facesToCheck.push_back(e.halfedge().face());
    if (!e.isBoundary()) {
      facesToCheck.push_back(e.halfedge().twin().face());
    }
  } else {
    HalfedgePtr he = mesh->allHalfedge(localInd - halfedgePickIndStart);
    facesToCheck.push_back(he.face());
    if (he.twin().isReal()) {
      facesToCheck.push_back(he.twin().face());
    }
  }

  // Get the coordinates of the mouse (in its CURRENT position)
  ImVec2 p = ImGui::GetMousePos();
  ImGuiIO& io = ImGui::GetIO();
  glm::vec2 mouseCoords{(2.0 * p.x) / view::windowWidth - 1.0, (2.0 * p.y) / view::windowHeight - 1.0};
  mouseCoords.y *= -1;

  // Test all candidate faces
  for (FacePtr f : facesToCheck) {
    bool hitTri = screenSpaceTriangleTest(f, mouseCoords, baryCoordOut);
    if (hitTri) {
      fOut = f;
      return;
    }
  }

  // None found, no intersection
  fOut = FacePtr();
}
*/


void SurfaceMesh::buildVertexInfoGui(size_t vInd) {
  ImGui::TextUnformatted(("Vertex #" + std::to_string(vInd)).c_str());

  std::stringstream buffer;
  buffer << triMesh.vertices[vInd].position();
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildVertexInfoGUI(vInd);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildFaceInfoGui(size_t fInd) {
  ImGui::TextUnformatted(("Face #" + std::to_string(fInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildFaceInfoGUI(fInd);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildEdgeInfoGui(size_t eInd) {
  ImGui::TextUnformatted(("Edge #" + std::to_string(eInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildEdgeInfoGUI(eInd);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildHalfedgeInfoGui(size_t heInd) {
  ImGui::TextUnformatted(("Halfedge #" + std::to_string(heInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildHalfedgeInfoGUI(heInd);
  }

  ImGui::Indent(-20.);
}


void SurfaceMesh::drawUI() {
  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels

  if (ImGui::TreeNode(name.c_str())) {
    // enabled = true;

    // Print stats
    // TODO add support back for connected components, boundary, etc
    // TODO wrong for multiple connected components
    /*
    long long int nVerts = static_cast<long long int>(nVertices);
    long long int nConnComp = static_cast<long long int>(mesh->nConnectedComponents());
    bool hasBoundary = mesh->nBoundaryLoops() > 0;
    if (nConnComp > 1) {
      if (hasBoundary) {
        ImGui::Text("# verts: %lld  # components: %lld", nVerts, nConnComp);
        ImGui::Text("# boundary: %lu", static_cast<unsigned long>(mesh->nBoundaryLoops()));
      } else {
        ImGui::Text("# verts: %lld  # components: %lld", nVerts, nConnComp);
      }
    } else {
      if (hasBoundary) {
        ImGui::Text("# verts: %lld  # boundary: %lu", nVerts, static_cast<unsigned long>(mesh->nBoundaryLoops()));
      } else {
        long long int eulerCharacteristic = nVertices - nEdges + nFaces;
        long long int genus = (2 - eulerCharacteristic) / 2;
        ImGui::Text("# verts: %lld  genus: %lld", nVerts, genus);
      }
    }
    */


    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();

    // Options popup
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      // Transform
      if (ImGui::BeginMenu("Transform")) {
        if (ImGui::MenuItem("Center")) centerBoundingBox();
        if (ImGui::MenuItem("Reset")) resetTransform();
        ImGui::EndMenu();
      }

      // Quantities
      if (ImGui::MenuItem("Clear Quantities")) removeAllQuantities();


      ImGui::EndPopup();
    }

    ImGui::ColorEdit3("Color", (float*)&surfaceColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();

    { // Flat shading or smooth shading?
      if(ImGui::Checkbox("Smooth", &ui_smoothshade)) {
        if(ui_smoothshade) {
          setShadeStyle(ShadeStyle::SMOOTH);
        } else {
          setShadeStyle(ShadeStyle::FLAT);
        }
      }
      ImGui::SameLine();
    }

    { // Edge width
      ImGui::Checkbox("Edges", &showEdges);
      if (showEdges) {
        edgeWidth = 0.01;
      } else {
        edgeWidth = 0.0;
      }
    }

    // Draw the quantities
    for (auto x : quantities) {
      x.second->drawUI();
    }

    ImGui::TreePop();
  }
  ImGui::PopID();
}
  
void SurfaceMesh::setShadeStyle(ShadeStyle newShadeStyle) {
  ui_smoothshade = (newShadeStyle == ShadeStyle::SMOOTH);
  shadeStyle = newShadeStyle;
  deleteProgram();
}

double SurfaceMesh::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding
  // box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (HalfedgeMesh::Vertex& vert : triMesh.vertices) {
    glm::vec3 p = vert.position();
    glm::vec3 transPos = glm::vec3(objectTransform * glm::vec4(p.x, p.y, p.z, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(transPos - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> SurfaceMesh::boundingBox() {
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (HalfedgeMesh::Vertex& vert : triMesh.vertices) {
    glm::vec3 p = glm::vec3(objectTransform * glm::vec4(vert.position(), 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  // Respect object transform
  min = glm::vec3(glm::vec4(min.x, min.y, min.z, 1.0));
  max = glm::vec3(glm::vec4(max.x, max.y, max.z, 1.0));

  return std::make_tuple(min, max);
}

/* TODO resurrect
VertexPtr SurfaceMesh::selectVertex() {

  // Make sure we can see edges
  edgeWidth = 0.01;
  enabled = true;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();
  VertexPtr returnVert;
  int iV = 0;

  // Register the callback which creates the UI and does the hard work
  focusedPopupUI = [&]() {
    { // Create a window with instruction and a close button.
      static bool showWindow = true;
      ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
      ImGui::Begin("Select vertex", &showWindow);

      ImGui::PushItemWidth(300);
      ImGui::TextUnformatted("Hold ctrl and left-click to select a vertex");
      ImGui::Separator();

      // Pick by number
      ImGui::PushItemWidth(300);
      ImGui::InputInt("index", &iV);
      if (ImGui::Button("Select by index")) {
        if (iV >= 0 && (size_t)iV < mesh->nVertices()) {
          returnVert = mesh->vertex(iV);
          focusedPopupUI = nullptr;
        }
      }
      ImGui::PopItemWidth();

      ImGui::Separator();
      if (ImGui::Button("Abort")) {
        focusedPopupUI = nullptr;
      }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && !io.WantCaptureMouse && ImGui::IsMouseClicked(0)) {
      if (pick::pickIsFromThisFrame) {
        size_t pickInd;
        Structure* pickS = pick::getCurrentPickElement(pickInd);

        if (pickS == this) {
          VertexPtr v;
          EdgePtr e;
          FacePtr f;
          HalfedgePtr he;
          getPickedElement(pickInd, v, f, e, he);

          if (v != VertexPtr()) {
            returnVert = v;
            focusedPopupUI = nullptr;
          }
        }
      }
    }

    ImGui::End();
  };


  // Re-enter main loop
  while (focusedPopupUI) {
    mainLoopIteration();
  }

  // Restore the old context
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);

  if (returnVert == VertexPtr()) return returnVert;

  return transfer.vMapBack[returnVert];
}


FacePtr SurfaceMesh::selectFace() {

  // Make sure we can see edges
  edgeWidth = 0.01;
  enabled = true;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();
  FacePtr returnFace;
  int iF = 0;

  // Register the callback which creates the UI and does the hard work
  focusedPopupUI = [&]() {
    { // Create a window with instruction and a close button.
      static bool showWindow = true;
      ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
      ImGui::Begin("Select face", &showWindow);

      ImGui::PushItemWidth(300);
      ImGui::TextUnformatted("Hold ctrl and left-click to select a face");
      ImGui::Separator();

      // Pick by number
      ImGui::PushItemWidth(300);
      ImGui::InputInt("index", &iF);
      if (ImGui::Button("Select by index")) {
        if (iF >= 0 && (size_t)iF < mesh->nFaces()) {
          returnFace = mesh->face(iF);
          focusedPopupUI = nullptr;
        }
      }
      ImGui::PopItemWidth();

      ImGui::Separator();
      if (ImGui::Button("Abort")) {
        focusedPopupUI = nullptr;
      }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && !io.WantCaptureMouse && ImGui::IsMouseClicked(0)) {
      if (pick::pickIsFromThisFrame) {
        size_t pickInd;
        Structure* pickS = pick::getCurrentPickElement(pickInd);

        if (pickS == this) {
          VertexPtr v;
          EdgePtr e;
          FacePtr f;
          HalfedgePtr he;
          getPickedElement(pickInd, v, f, e, he);

          if (f != FacePtr()) {
            returnFace = f;
            focusedPopupUI = nullptr;
          }
        }
      }
    }

    ImGui::End();
  };


  // Re-enter main loop
  while (focusedPopupUI) {
    mainLoopIteration();
  }

  // Restore the old context
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);

  if (returnFace == FacePtr()) return returnFace;

  return transfer.fMapBack[returnFace];
}
*/

void SurfaceMesh::updateVertexPositions(const std::vector<glm::vec3>& newPositions) {

  triMesh.updateVertexPositions(newPositions);

  // Rebuild any necessary quantities
  deleteProgram();
  prepare();
}

SurfaceQuantity::SurfaceQuantity(std::string name_, SurfaceMesh* mesh_) : name(name_), parent(mesh_) {}
SurfaceQuantityThatDrawsFaces::SurfaceQuantityThatDrawsFaces(std::string name_, SurfaceMesh* mesh_)
    : SurfaceQuantity(name_, mesh_) {}
SurfaceQuantity::~SurfaceQuantity() {}

void SurfaceMesh::addSurfaceQuantity(SurfaceQuantity* quantity) {
  std::shared_ptr<SurfaceQuantity> ptr;
  ptr.reset(quantity);
  addSurfaceQuantity(ptr);
}

void SurfaceMesh::addSurfaceQuantity(std::shared_ptr<SurfaceQuantity> quantity) {
  // Delete old if in use
  bool wasEnabled = false;
  if (quantities.find(quantity->name) != quantities.end()) {
    wasEnabled = quantities[quantity->name]->isEnabled();
    removeQuantity(quantity->name);
  }

  // Store
  quantities[quantity->name] = quantity;

  // Re-enable the quantity if we're replacing an enabled quantity
  if (wasEnabled) {
    quantity->enable();
  }
}


std::shared_ptr<SurfaceQuantity> SurfaceMesh::getSurfaceQuantity(std::string name, bool errorIfAbsent) {
  // Check if exists
  if (quantities.find(name) == quantities.end()) {
    if (errorIfAbsent) {
      polyscope::error("No quantity named " + name + " registered");
    }
    return nullptr;
  }

  return quantities[name];
}


void SurfaceMesh::addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  std::shared_ptr<SurfaceCountQuantity> q = std::make_shared<SurfaceCountVertexQuantity>(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  std::shared_ptr<SurfaceCountQuantity> q = std::make_shared<SurfaceCountFaceQuantity>(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addIsolatedVertexScalarQuantity(std::string name,
                                                  const std::vector<std::pair<size_t, double>>& values) {
  std::shared_ptr<SurfaceCountQuantity> q = std::make_shared<SurfaceIsolatedScalarVertexQuantity>(name, values, this);
  addSurfaceQuantity(q);
}


// void SurfaceMesh::addInputCurveQuantity(std::string name) {
// SurfaceInputCurveQuantity* q = new SurfaceInputCurveQuantity(name, this);
// addSurfaceQuantity(q);
//}


void SurfaceMesh::removeQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return;
  }

  std::shared_ptr<SurfaceQuantity> q = quantities[name];
  quantities.erase(name);
  if (activeSurfaceQuantity == q.get()) {
    clearActiveSurfaceQuantity();
  }
}

void SurfaceMesh::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
}

void SurfaceMesh::setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q) {
  if (activeSurfaceQuantity == q) return;
  clearActiveSurfaceQuantity();
  activeSurfaceQuantity = q;
  q->enable();
}

void SurfaceMesh::clearActiveSurfaceQuantity() {
  deleteProgram();
  if (activeSurfaceQuantity != nullptr) {
    SurfaceQuantityThatDrawsFaces* oldActiveQuantity = activeSurfaceQuantity;
    activeSurfaceQuantity = nullptr;
    // do this after setting to nullptr in case disable() implementation recurses
    oldActiveQuantity->disable();
  }
}

void SurfaceQuantity::buildVertexInfoGUI(size_t vInd) {}
void SurfaceQuantity::buildFaceInfoGUI(size_t fInd) {}
void SurfaceQuantity::buildEdgeInfoGUI(size_t eInd) {}
void SurfaceQuantity::buildHalfedgeInfoGUI(size_t heInd) {}

bool SurfaceQuantity::isEnabled() { return enabled; }

void SurfaceQuantity::enable() { enabled = true; }
void SurfaceQuantity::disable() { enabled = false; }
void SurfaceQuantity::setEnabled(bool newEnabled) {
  if (enabled == false && newEnabled == true) {
    enable();
  } else if (enabled == true && newEnabled == false) {
    disable();
  }
}

void SurfaceQuantityThatDrawsFaces::setProgramValues(gl::GLProgram* program) {}

void SurfaceQuantityThatDrawsFaces::enable() {
  enabled = true;
  parent->setActiveSurfaceQuantity(this);
}

void SurfaceQuantityThatDrawsFaces::disable() {
  enabled = false;
  parent->clearActiveSurfaceQuantity();
}

} // namespace polyscope

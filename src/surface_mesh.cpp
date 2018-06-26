#include "polyscope/surface_mesh.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

// Quantities
#include "polyscope/surface_color_quantity.h"
#include "polyscope/surface_count_quantity.h"
#include "polyscope/surface_distance_quantity.h"
#include "polyscope/surface_input_curve_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_selection_quantity.h"
#include "polyscope/surface_subset_quantity.h"
#include "polyscope/surface_vector_quantity.h"

#include "imgui.h"

using namespace geometrycentral;
using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string SurfaceMesh::structureTypeName = "Surface Mesh";

SurfaceMesh::SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_)
    : Structure(name, SurfaceMesh::structureTypeName) {

  originalMesh = geometry_->getMesh();
  originalGeometry = geometry_;

  // Copy the mesh and save the transfer object
  mesh = geometry_->getMesh()->copy(transfer);
  geometry = geometry_->copyUsingTransfer(transfer);

  // Colors
  baseColor = getNextStructureColor();
  surfaceColor = baseColor;
  colorManager = SubColorManager(baseColor);

  prepare();
  preparePick();

  if (options::autocenterStructures) {
    centerBoundingBox();
  }
}

SurfaceMesh::~SurfaceMesh() {
  deleteProgram();

  // Delete quantities
  for (auto x : quantities) {
    delete x.second;
  }
}

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
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
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
  pickProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

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

  // Populate draw buffers
  fillGeometryBuffers();
}

void SurfaceMesh::preparePick() {

  // Create a new program
  pickProgram = new gl::GLProgram(&PICK_SURFACE_VERT_SHADER, &PICK_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Get element indices
  size_t totalPickElements = mesh->nVertices() + mesh->nFaces() + mesh->nEdges() + mesh->nHalfedges();

  // In "local" indices, indexing elements only within this mesh, used for reading later
  facePickIndStart = mesh->nVertices();
  edgePickIndStart = facePickIndStart + mesh->nFaces();
  halfedgePickIndStart = edgePickIndStart + mesh->nEdges();

  // In "global" indices, indexing all elements in the scene, used to fill buffers for drawing here
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);
  size_t faceGlobalPickIndStart = pickStart + mesh->nVertices();
  size_t edgeGlobalPickIndStart = faceGlobalPickIndStart + mesh->nFaces();
  size_t halfedgeGlobalPickIndStart = edgeGlobalPickIndStart + mesh->nEdges();

  // Fill buffers
  std::vector<Vector3> positions;
  std::vector<Vector3> bcoord;
  std::vector<std::array<Vector3, 3>> vertexColors, edgeColors, halfedgeColors;
  std::vector<Vector3> faceColor;

  // Use natural indices
  vInd = mesh->getVertexIndices();
  fInd = mesh->getFaceIndices();
  eInd = mesh->getEdgeIndices();
  heInd = mesh->getHalfedgeIndices();

  for (FacePtr f : mesh->faces()) {

    VertexPtr baseVert = f.halfedge().vertex();

    // Triangulate faces
    HalfedgePtr prevHe = f.halfedge();
    HalfedgePtr oppositeHe = f.halfedge().next();

    do {

      // Test if this the first or last triangle in the triangulation
      // (otherwise, edges are artificial and shouldn't have pick indices)
      bool isFirst = prevHe == f.halfedge();
      bool isLast = oppositeHe.next().next() == f.halfedge();

      // Build all quantities
      std::array<Vector3, 3> vColor, eColor, heColor;

      // Positions
      positions.push_back(geometry->position(baseVert));
      positions.push_back(geometry->position(oppositeHe.vertex()));
      positions.push_back(geometry->position(oppositeHe.next().vertex()));

      // Face index color
      Vector3 faceIndColor = pick::indToVec(fInd[f] + faceGlobalPickIndStart);
      faceColor.push_back(faceIndColor);
      faceColor.push_back(faceIndColor);
      faceColor.push_back(faceIndColor);

      // Vertex index colors
      vColor[0] = pick::indToVec(vInd[baseVert] + pickStart);
      vColor[1] = pick::indToVec(vInd[oppositeHe.vertex()] + pickStart);
      vColor[2] = pick::indToVec(vInd[oppositeHe.twin().vertex()] + pickStart);

      // Edge index color
      if (isFirst) {
        eColor[0] = pick::indToVec(eInd[prevHe.edge()] + edgeGlobalPickIndStart);
      } else {
        eColor[0] = faceIndColor;
      }
      eColor[1] = pick::indToVec(eInd[oppositeHe.edge()] + edgeGlobalPickIndStart);
      if (isLast) {
        eColor[2] = pick::indToVec(eInd[oppositeHe.next().edge()] + edgeGlobalPickIndStart);
      } else {
        eColor[2] = faceIndColor;
      }

      // Halfedge index color
      if (isFirst) {
        heColor[0] = pick::indToVec(heInd[prevHe] + halfedgeGlobalPickIndStart);
      } else {
        heColor[0] = faceIndColor;
      }
      heColor[1] = pick::indToVec(heInd[oppositeHe] + halfedgeGlobalPickIndStart);
      if (isLast) {
        heColor[2] = pick::indToVec(heInd[oppositeHe.next()] + halfedgeGlobalPickIndStart);
      } else {
        heColor[2] = faceIndColor;
      }

      // Push three copies of the values needed at each vertex
      for (int j = 0; j < 3; j++) {
        vertexColors.push_back(vColor);
        edgeColors.push_back(eColor);
        halfedgeColors.push_back(heColor);
      }

      // Barycoords
      bcoord.push_back(Vector3{1.0, 0.0, 0.0});
      bcoord.push_back(Vector3{0.0, 1.0, 0.0});
      bcoord.push_back(Vector3{0.0, 0.0, 1.0});

      prevHe = prevHe.next();
      oppositeHe = oppositeHe.next();
    } while (oppositeHe.next() != f.halfedge());
  }

  // Store data in buffers
  pickProgram->setAttribute("a_position", positions);
  pickProgram->setAttribute("a_barycoord", bcoord);
  pickProgram->setAttribute<Vector3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute<Vector3, 3>("a_edgeColors", edgeColors);
  pickProgram->setAttribute<Vector3, 3>("a_halfedgeColors", halfedgeColors);
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
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> bcoord;
  VertexData<Vector3> vertexNormals;
  geometry->getVertexNormals(vertexNormals);

  // Implicitly triangulate
  for (FacePtr f : mesh->faces()) {

    VertexPtr baseVert = f.halfedge().vertex();
    HalfedgePtr prevHe = f.halfedge();
    HalfedgePtr oppositeHe = f.halfedge().next();

    do {

      // Build all quantities
      std::array<Vector3, 3> vColor, eColor, heColor;

      // Positions
      positions.push_back(geometry->position(baseVert));
      positions.push_back(geometry->position(oppositeHe.vertex()));
      positions.push_back(geometry->position(oppositeHe.next().vertex()));

      // Normals
      normals.push_back(vertexNormals[baseVert]);
      normals.push_back(vertexNormals[oppositeHe.vertex()]);
      normals.push_back(vertexNormals[oppositeHe.next().vertex()]);

      // Barycoords
      bcoord.push_back(Vector3{1.0, 0.0, 0.0});
      bcoord.push_back(Vector3{0.0, 1.0, 0.0});
      bcoord.push_back(Vector3{0.0, 0.0, 1.0});

      prevHe = prevHe.next();
      oppositeHe = oppositeHe.next();
    } while (oppositeHe.next() != f.halfedge());
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
} // namespace polyscope

void SurfaceMesh::fillGeometryBuffersFlat() {
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> bcoord;

  FaceData<Vector3> faceNormals;
  geometry->getFaceNormals(faceNormals);
  
  // Implicitly triangulate
  for (FacePtr f : mesh->faces()) {

    VertexPtr baseVert = f.halfedge().vertex();
    HalfedgePtr prevHe = f.halfedge();
    HalfedgePtr oppositeHe = f.halfedge().next();

    do {

      // Build all quantities
      std::array<Vector3, 3> vColor, eColor, heColor;

      // Positions
      positions.push_back(geometry->position(baseVert));
      positions.push_back(geometry->position(oppositeHe.vertex()));
      positions.push_back(geometry->position(oppositeHe.next().vertex()));

      // Normals
      Vector3 faceNormal = faceNormals[f];
      normals.push_back(faceNormal);
      normals.push_back(faceNormal);
      normals.push_back(faceNormal);

      // Barycoords
      bcoord.push_back(Vector3{1.0, 0.0, 0.0});
      bcoord.push_back(Vector3{0.0, 1.0, 0.0});
      bcoord.push_back(Vector3{0.0, 0.0, 1.0});

      prevHe = prevHe.next();
      oppositeHe = oppositeHe.next();
    } while (oppositeHe.next() != f.halfedge());
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
    buildVertexInfoGui(mesh->vertex(localPickID));
  } else if (localPickID < edgePickIndStart) {
    buildFaceInfoGui(mesh->face(localPickID - facePickIndStart));
  } else if (localPickID < halfedgePickIndStart) {
    buildEdgeInfoGui(mesh->edge(localPickID - edgePickIndStart));
  } else {
    buildHalfedgeInfoGui(mesh->allHalfedge(localPickID - halfedgePickIndStart));
  }
}

void SurfaceMesh::getPickedElement(size_t localPickID, VertexPtr& vOut, FacePtr& fOut, EdgePtr& eOut,
                                   HalfedgePtr& heOut) {

  vOut = VertexPtr();
  fOut = FacePtr();
  eOut = EdgePtr();
  heOut = HalfedgePtr();

  if (localPickID < facePickIndStart) {
    vOut = mesh->vertex(localPickID);
  } else if (localPickID < edgePickIndStart) {
    fOut = mesh->face(localPickID - facePickIndStart);
  } else if (localPickID < halfedgePickIndStart) {
    eOut = mesh->edge(localPickID - edgePickIndStart);
  } else {
    heOut = mesh->allHalfedge(localPickID - halfedgePickIndStart);
  }
}


Vector2 SurfaceMesh::projectToScreenSpace(Vector3 coord) {

  glm::mat4 viewMat = getModelView();
  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  glm::vec4 coord4(coord.x, coord.y, coord.z, 1.0);
  glm::vec4 screenPoint = projMat * viewMat * coord4;

  return Vector2{screenPoint.x, screenPoint.y} / screenPoint.w;
}

bool SurfaceMesh::screenSpaceTriangleTest(FacePtr f, Vector2 testCoords, Vector3& bCoordOut) {

  // Get points in screen space
  Vector2 p0 = projectToScreenSpace(geometry->position(f.halfedge().vertex()));
  Vector2 p1 = projectToScreenSpace(geometry->position(f.halfedge().next().vertex()));
  Vector2 p2 = projectToScreenSpace(geometry->position(f.halfedge().next().next().vertex()));

  // Make sure triangle is positively oriented
  if (cross(p1 - p0, p2 - p0).z < 0) {
    cout << "triangle not positively oriented" << endl;
    return false;
  }

  // Test the point
  Vector2 v0 = p1 - p0;
  Vector2 v1 = p2 - p0;
  Vector2 vT = testCoords - p0;

  double dot00 = dot(v0, v0);
  double dot01 = dot(v0, v1);
  double dot0T = dot(v0, vT);
  double dot11 = dot(v1, v1);
  double dot1T = dot(v1, vT);

  double denom = 1 / (dot00 * dot11 - dot01 * dot01);
  double v = (dot11 * dot0T - dot01 * dot1T) * denom;
  double w = (dot00 * dot1T - dot01 * dot0T) * denom;

  // Check if point is in triangle
  bool inTri = (v >= 0) && (w >= 0) && (v + w < 1);
  if (!inTri) {
    return false;
  }

  bCoordOut = Vector3{1.0 - v - w, v, w};
  return true;
}


void SurfaceMesh::getPickedFacePoint(FacePtr& fOut, Vector3& baryCoordOut) {

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
  Vector2 mouseCoords{(2.0 * p.x) / view::windowWidth - 1.0, (2.0 * p.y) / view::windowHeight - 1.0};
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


void SurfaceMesh::buildVertexInfoGui(VertexPtr v) {
  ImGui::TextUnformatted(("Vertex #" + std::to_string(vInd[v])).c_str());

  std::stringstream buffer;
  buffer << geometry->position(v);
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildInfoGUI(v);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildFaceInfoGui(FacePtr f) {
  ImGui::TextUnformatted(("Face #" + std::to_string(fInd[f])).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildInfoGUI(f);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildEdgeInfoGui(EdgePtr e) {
  ImGui::TextUnformatted(("Edge #" + std::to_string(eInd[e])).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildInfoGUI(e);
  }

  ImGui::Indent(-20.);
}

void SurfaceMesh::buildHalfedgeInfoGui(HalfedgePtr he) {
  ImGui::TextUnformatted(("Halfedge #" + std::to_string(heInd[he])).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto x : quantities) {
    x.second->buildInfoGUI(he);
  }

  ImGui::Indent(-20.);
}


void SurfaceMesh::drawUI() {
  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels

  if (ImGui::TreeNode(name.c_str())) {
    // enabled = true;

    // Print stats
    // TODO wrong for multiple connected components
    long long int nVerts = static_cast<long long int>(mesh->nVertices());
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
        long long int eulerCharacteristic = mesh->nVertices() - mesh->nEdges() + mesh->nFaces();
        long long int genus = (2 - eulerCharacteristic) / 2;
        ImGui::Text("# verts: %lld  genus: %lld", nVerts, genus);
      }
    }


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
      ImGui::Checkbox("Smooth", &ui_smoothshade);
      if (ui_smoothshade && shadeStyle == ShadeStyle::FLAT) {
        shadeStyle = ShadeStyle::SMOOTH;
        deleteProgram();
      }
      if (!ui_smoothshade && shadeStyle == ShadeStyle::SMOOTH) {
        shadeStyle = ShadeStyle::FLAT;
        deleteProgram();
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

double SurfaceMesh::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding
  // box
  auto bound = boundingBox();
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (VertexPtr v : mesh->vertices()) {
    Vector3 p = geometry->position(v);
    Vector3 transPos = fromGLM(glm::vec3(objectTransform * glm::vec4(p.x, p.y, p.z, 1.0)));
    lengthScale = std::max(lengthScale, geometrycentral::norm2(transPos - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> SurfaceMesh::boundingBox() {
  Vector3 min = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (VertexPtr v : mesh->vertices()) {
    min = geometrycentral::componentwiseMin(min, geometry->position(v));
    max = geometrycentral::componentwiseMax(max, geometry->position(v));
  }

  // Respect object transform
  min = fromGLM(glm::vec3(objectTransform * glm::vec4(min.x, min.y, min.z, 1.0)));
  max = fromGLM(glm::vec3(objectTransform * glm::vec4(max.x, max.y, max.z, 1.0)));

  return std::make_tuple(min, max);
}

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


void SurfaceMesh::updateGeometryPositions(Geometry<Euclidean>* newGeometry) {

  VertexData<Vector3> newPositions;
  newGeometry->getVertexPositions(newPositions);

  VertexData<Vector3> myNewPositions = transfer.transfer(newPositions);
  for (VertexPtr v : mesh->vertices()) {
    geometry->position(v) = myNewPositions[v];
  }

  // Rebuild any necessary quantities
  deleteProgram();
  prepare();
}

SurfaceQuantity::SurfaceQuantity(std::string name_, SurfaceMesh* mesh_) : name(name_), parent(mesh_) {}
SurfaceQuantityThatDrawsFaces::SurfaceQuantityThatDrawsFaces(std::string name_, SurfaceMesh* mesh_)
    : SurfaceQuantity(name_, mesh_) {}
SurfaceQuantity::~SurfaceQuantity() {}

void SurfaceMesh::addSurfaceQuantity(SurfaceQuantity* quantity) {
  // Delete old if in use
  bool wasEnabled = false;
  if (quantities.find(quantity->name) != quantities.end()) {
    wasEnabled = quantities[quantity->name]->enabled;
    removeQuantity(quantity->name);
  }

  // Store
  quantities[quantity->name] = quantity;

  // Re-enable the quantity if we're replacing an enabled quantity
  if (wasEnabled) {
    quantity->enabled = true;
  }
}

void SurfaceMesh::addSurfaceQuantity(SurfaceQuantityThatDrawsFaces* quantity) {
  // Delete old if in use
  bool wasEnabled = false;
  if (quantities.find(quantity->name) != quantities.end()) {
    wasEnabled = quantities[quantity->name]->enabled;
    removeQuantity(quantity->name);
  }

  // Store
  quantities[quantity->name] = quantity;

  // Re-enable the quantity if we're replacing an enabled quantity
  if (wasEnabled) {
    quantity->enabled = true;
    setActiveSurfaceQuantity(quantity);
  }
}

SurfaceQuantity* SurfaceMesh::getSurfaceQuantity(std::string name, bool errorIfAbsent) {
  // Check if exists
  if (quantities.find(name) == quantities.end()) {
    if (errorIfAbsent) {
      polyscope::error("No quantity named " + name + " registered");
    }
    return nullptr;
  }

  return quantities[name];
}


void SurfaceMesh::addQuantity(std::string name, VertexData<double>& value, DataType type) {
  SurfaceScalarQuantity* q = new SurfaceScalarVertexQuantity(name, value, this, type);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addQuantity(std::string name, FaceData<double>& value, DataType type) {
  SurfaceScalarQuantity* q = new SurfaceScalarFaceQuantity(name, value, this, type);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addQuantity(std::string name, EdgeData<double>& value, DataType type) {
  SurfaceScalarQuantity* q = new SurfaceScalarEdgeQuantity(name, value, this, type);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addQuantity(std::string name, HalfedgeData<double>& value, DataType type) {
  SurfaceScalarQuantity* q = new SurfaceScalarHalfedgeQuantity(name, value, this, type);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addDistanceQuantity(std::string name, VertexData<double>& distances) {
  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(name, distances, this, false);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addSignedDistanceQuantity(std::string name, VertexData<double>& distances) {
  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(name, distances, this, true);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addColorQuantity(std::string name, VertexData<Vector3>& value) {
  SurfaceColorQuantity* q = new SurfaceColorVertexQuantity(name, value, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addColorQuantity(std::string name, FaceData<Vector3>& value) {
  SurfaceColorQuantity* q = new SurfaceColorFaceQuantity(name, value, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addCountQuantity(std::string name, std::vector<std::pair<VertexPtr, int>>& values) {
  SurfaceCountQuantity* q = new SurfaceCountVertexQuantity(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addIsolatedVertexQuantity(std::string name, std::vector<std::pair<VertexPtr, double>>& values) {
  SurfaceCountQuantity* q = new SurfaceIsolatedScalarVertexQuantity(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addCountQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values) {
  SurfaceCountQuantity* q = new SurfaceCountFaceQuantity(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addSubsetQuantity(std::string name, EdgeData<char>& subset) {
  SurfaceEdgeSubsetQuantity* q = new SurfaceEdgeSubsetQuantity(name, subset, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVertexSelectionQuantity(std::string name, VertexData<char>& initialMembership) {
  SurfaceSelectionVertexQuantity* q = new SurfaceSelectionVertexQuantity(name, initialMembership, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addInputCurveQuantity(std::string name) {
  SurfaceInputCurveQuantity* q = new SurfaceInputCurveQuantity(name, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVectorQuantity(std::string name, VertexData<Vector3>& value, VectorType vectorType) {
  SurfaceVectorQuantity* q = new SurfaceVertexVectorQuantity(name, value, this, vectorType);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVectorQuantity(std::string name, FaceData<Vector3>& value, VectorType vectorType) {
  SurfaceVectorQuantity* q = new SurfaceFaceVectorQuantity(name, value, this, vectorType);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVectorQuantity(std::string name, FaceData<Complex>& value, int nSym, VectorType vectorType) {
  SurfaceVectorQuantity* q = new SurfaceFaceIntrinsicVectorQuantity(name, value, this, nSym, vectorType);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVectorQuantity(std::string name, VertexData<Complex>& value, int nSym, VectorType vectorType) {
  SurfaceVectorQuantity* q = new SurfaceVertexIntrinsicVectorQuantity(name, value, this, nSym, vectorType);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addVectorQuantity(std::string name, EdgeData<double>& value) {
  SurfaceVectorQuantity* q = new SurfaceOneFormIntrinsicVectorQuantity(name, value, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::removeQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return;
  }

  SurfaceQuantity* q = quantities[name];
  quantities.erase(name);
  if (activeSurfaceQuantity == q) {
    clearActiveSurfaceQuantity();
  }
  delete q;
}

void SurfaceMesh::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
}

void SurfaceMesh::setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q) {
  clearActiveSurfaceQuantity();
  activeSurfaceQuantity = q;
  q->enabled = true;
}

void SurfaceMesh::clearActiveSurfaceQuantity() {
  deleteProgram();
  if (activeSurfaceQuantity != nullptr) {
    activeSurfaceQuantity->enabled = false;
    activeSurfaceQuantity = nullptr;
  }
}


void SurfaceMesh::resetTransform() {
  objectTransform = glm::mat4(1.0);
  updateStructureExtents();
}

void SurfaceMesh::centerBoundingBox() {
  std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> bbox = boundingBox();
  Vector3 center = (std::get<1>(bbox) + std::get<0>(bbox)) / 2.0;
  glm::mat4x4 newTrans = glm::translate(glm::mat4x4(1.0), -glm::vec3(center.x, center.y, center.z));
  objectTransform = objectTransform * newTrans;
  updateStructureExtents();
}

glm::mat4 SurfaceMesh::getModelView() { return view::getCameraViewMatrix() * objectTransform; }

void SurfaceQuantity::buildInfoGUI(VertexPtr v) {}
void SurfaceQuantity::buildInfoGUI(FacePtr f) {}
void SurfaceQuantity::buildInfoGUI(EdgePtr e) {}
void SurfaceQuantity::buildInfoGUI(HalfedgePtr he) {}


void SurfaceQuantityThatDrawsFaces::setProgramValues(gl::GLProgram* program) {}

} // namespace polyscope

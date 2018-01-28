#include "polyscope/surface_mesh.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

// Quantities
#include "polyscope/surface_color_quantity.h"
#include "polyscope/surface_count_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
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
  // Copy the mesh and save the transfer object
  mesh = geometry_->getMesh()->copy(transfer);
  geometry = geometry_->copyUsingTransfer(transfer);

  // Colors
  baseColor = getNextStructureColor();
  surfaceColor = baseColor;
  colorManager = SubColorManager(baseColor);

  prepare();
  preparePick();
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
  glm::mat4 viewMat = view::getCameraViewMatrix();
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
  glm::mat4 viewMat = view::getCameraViewMatrix();
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

  if (!mesh->isSimplicial()) {
    // TODO. This should be entirely possible by adding some logic to avoid drawing pick colors for virtual
    // triangulation edges, but hasn't been implemented yet.
    error("Don't know how to pick from non-triangle mesh");
  }

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

    // Build all quantities
    std::array<Vector3, 3> vColor, eColor, heColor;
    size_t i = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {

      VertexPtr v = he.vertex();
      EdgePtr e = he.edge();

      // Want just one copy of positions and face color, so we can build it in the usual way
      positions.push_back(geometry->position(v));
      faceColor.push_back(pick::indToVec(fInd[f] + faceGlobalPickIndStart));

      vColor[i] = pick::indToVec(vInd[v] + pickStart);
      eColor[i] = pick::indToVec(eInd[e] + edgeGlobalPickIndStart);
      heColor[i] = pick::indToVec(heInd[he] + halfedgeGlobalPickIndStart);
      i++;
    }

    // Push three copies of the values needed at each vertex
    for (int j = 0; j < 3; j++) {
      vertexColors.push_back(vColor);
      edgeColors.push_back(eColor);
      halfedgeColors.push_back(heColor);
    }

    // Just one copy of barycoords needed
    bcoord.push_back(Vector3{1.0, 0.0, 0.0});
    bcoord.push_back(Vector3{0.0, 1.0, 0.0});
    bcoord.push_back(Vector3{0.0, 0.0, 1.0});
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
  for (FacePtr f : mesh->faces()) {
    // Implicitly triangulate
    Vector3 p0, p1;
    Vector3 n0, n1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 p2 = geometry->position(v);
      Vector3 n2 = vertexNormals[v];
      if (iP >= 2) {
        positions.push_back(p0);
        normals.push_back(n0);
        bcoord.push_back(Vector3{1.0, 0.0, 0.0});
        positions.push_back(p1);
        normals.push_back(n1);
        bcoord.push_back(Vector3{0.0, 1.0, 0.0});
        positions.push_back(p2);
        normals.push_back(n2);
        bcoord.push_back(Vector3{0.0, 0.0, 1.0});
      }
      p0 = p1;
      p1 = p2;
      n0 = n1;
      n1 = n2;
      iP++;
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
}

void SurfaceMesh::fillGeometryBuffersFlat() {
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> bcoord;

  FaceData<Vector3> faceNormals;
  geometry->getFaceNormals(faceNormals);
  for (FacePtr f : mesh->faces()) {
    // Implicitly triangulate
    Vector3 p0, p1;
    Vector3 n0, n1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 p2 = geometry->position(v);
      Vector3 n2 = faceNormals[f];
      if (iP >= 2) {
        positions.push_back(p0);
        normals.push_back(n0);
        bcoord.push_back(Vector3{1.0, 0.0, 0.0});
        positions.push_back(p1);
        normals.push_back(n1);
        bcoord.push_back(Vector3{0.0, 1.0, 0.0});
        positions.push_back(p2);
        normals.push_back(n2);
        bcoord.push_back(Vector3{0.0, 0.0, 1.0});
      }
      p0 = p1;
      p1 = p2;
      n0 = n1;
      n1 = n2;
      iP++;
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
    buildVertexInfoGui(mesh->vertex(localPickID));
  } else if (localPickID < edgePickIndStart) {
    buildFaceInfoGui(mesh->face(localPickID - facePickIndStart));
  } else if (localPickID < halfedgePickIndStart) {
    buildEdgeInfoGui(mesh->edge(localPickID - edgePickIndStart));
  } else {
    buildHalfedgeInfoGui(mesh->halfedge(localPickID - halfedgePickIndStart));
  }
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
    ImGui::ColorEdit3("Surface color", (float*)&surfaceColor, ImGuiColorEditFlags_NoInputs);

    { // Flat shading or smooth shading?
      ImGui::Checkbox("Smooth shade", &ui_smoothshade);
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
      ImGui::Checkbox("Show edges", &showEdges);
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
  } else {
    // enabled = false;
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
    lengthScale = std::max(lengthScale, geometrycentral::norm2(geometry->position(v) - center));
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

  return std::make_tuple(min, max);
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
    if(errorIfAbsent) {
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

void SurfaceMesh::addCountQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values) {
  SurfaceCountQuantity* q = new SurfaceCountFaceQuantity(name, values, this);
  addSurfaceQuantity(q);
}

void SurfaceMesh::addSubsetQuantity(std::string name, EdgeData<char>& subset) {
  SurfaceEdgeSubsetQuantity* q = new SurfaceEdgeSubsetQuantity(name, subset, this);
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

void SurfaceQuantity::buildInfoGUI(VertexPtr v) {}
void SurfaceQuantity::buildInfoGUI(FacePtr f) {}
void SurfaceQuantity::buildInfoGUI(EdgePtr e) {}
void SurfaceQuantity::buildInfoGUI(HalfedgePtr he) {}

void SurfaceQuantityThatDrawsFaces::setProgramValues(gl::GLProgram* program) {}

} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/volume_mesh.h"

#include "polyscope/color_management.h"
#include "polyscope/combining_hash_functions.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <unordered_map>
#include <utility>

using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string VolumeMesh::structureTypeName = "Volume Mesh";

// clang-format off
const std::vector<std::vector<std::array<size_t, 3>>> VolumeMesh::stencilTet = 
 {
   {{0,2,1}}, 
   {{0,1,3}}, 
   {{0,3,2}}, 
   {{1,2,3}},
 };

const std::vector<std::vector<std::array<size_t, 3>>> VolumeMesh::stencilHex = 
  // numbered like in this diagram, except with 6/7 swapped
  // https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf
 {
   {{2,1,0}, {2,0,3}}, 
   {{4,0,1}, {4,1,5}}, 
   {{5,1,2}, {5,2,6}}, 
   {{7,3,0}, {7,0,4}}, 
   {{6,2,3}, {6,3,7}}, 
   {{7,4,5}, {7,5,6}}, 
 };
// clang-format on

VolumeMesh::VolumeMesh(std::string name, const std::vector<glm::vec3>& vertexPositions,
                       const std::vector<std::array<int64_t, 8>>& cellIndices)
    : QuantityStructure<VolumeMesh>(name, typeName()), vertices(vertexPositions), cells(cellIndices),
      color(uniquePrefix() + "color", getNextUniqueColor()),
      interiorColor(uniquePrefix() + "interiorColor", color.get()),
      edgeColor(uniquePrefix() + "edgeColor", glm::vec3{0., 0., 0.}), material(uniquePrefix() + "material", "clay"),
      edgeWidth(uniquePrefix() + "edgeWidth", 0.) {
  cullWholeElements.setPassive(true);

  // set the interior color to be a desaturated version of the normal one
  glm::vec3 desatColorHSV = RGBtoHSV(color.get());
  desatColorHSV.y *= 0.3;
  interiorColor.setPassive(HSVtoRGB(desatColorHSV));


  computeCounts();
  computeGeometryData();
}


void VolumeMesh::computeCounts() {

  // ==== Populate counts
  nFacesCount = 0;
  nFacesTriangulationCount = 0;

  vertexDataSize = nVertices();
  edgeDataSize = 0; // TODO
  faceDataSize = 0; // TODO
  cellDataSize = nCells();

  // ==== Populate interior/exterior faces
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<int64_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);
    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      nFacesCount++;
      nFacesTriangulationCount += face.size();
    }
  }

  // == Step 1: count occurences of each face
  std::unordered_map<std::array<int64_t, 4>, int, polyscope::hash_combine::hash<std::array<int64_t, 4>>> faceCounts;

  std::set<size_t> faceInds; // Scratch map

  // Build a sorted list of the indices of this face

  // Helper to build a sorted-index array for a face
  auto generateSortedFace = [&](const std::array<int64_t, 8>& cell, const std::vector<std::array<size_t, 3>>& face) {
    faceInds.clear();
    for (const std::array<size_t, 3>& tri : face) {
      for (int j = 0; j < 3; j++) {
        faceInds.insert(cell[tri[j]]);
      }
    }
    std::array<int64_t, 4> sortedFace{-1, -1, -1, -1};
    int j = 0;
    for (size_t ind : faceInds) {
      sortedFace[j] = ind;
      j++;
    }
    return sortedFace;
  };

  // Iterate over cells
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<int64_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      std::array<int64_t, 4> sortedFace = generateSortedFace(cell, face);
      // Add to the count
      if (faceCounts.find(sortedFace) == faceCounts.end()) {
        faceCounts[sortedFace] = 0;
      }
      faceCounts[sortedFace]++;
    }
  }

  // Iterate a second time; all faces which were seen more than once are inteior
  faceIsInterior.clear();
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<int64_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      std::array<int64_t, 4> sortedFace = generateSortedFace(cell, face);
      faceIsInterior.push_back(faceCounts[sortedFace] > 1);
    }
  }
}

void VolumeMesh::computeGeometryData() {

  /*
  const glm::vec3 zero{0., 0., 0.};

  // Reset face-valued
  faceNormals.resize(nFaces());
  faceAreas.resize(nFaces());

  // Reset vertex-valued
  vertexNormals.resize(nVertices());
  std::fill(vertexNormals.begin(), vertexNormals.end(), zero);
  vertexAreas.resize(nVertices());
  std::fill(vertexAreas.begin(), vertexAreas.end(), 0);

  // Reset edge-valued
  edgeLengths.resize(nEdges());

  // Loop over faces to compute face-valued quantities
  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();

    glm::vec3 fN = zero;
    double fA = 0;
    if (face.size() == 3) {
      glm::vec3 pA = vertices[face[0]];
      glm::vec3 pB = vertices[face[1]];
      glm::vec3 pC = vertices[face[2]];

      fN = glm::cross(pB - pA, pC - pA);
      fA = 0.5 * glm::length(fN);
    } else if (face.size() > 3) {

      glm::vec3 pRoot = vertices[face[0]];
      for (size_t j = 0; j < D; j++) {
        glm::vec3 pA = vertices[face[j]];
        glm::vec3 pB = vertices[face[(j + 1) % D]];
        glm::vec3 pC = vertices[face[(j + 2) % D]];

        fN += glm::cross(pC - pB, pA - pB);

        // _some_ definition of area for a non-triangular face
        if (j != 0 && j != (D - 1)) {
          fA += 0.5 * glm::length(glm::cross(pA - pRoot, pB - pRoot));
        }
      }
    }

    // Set face values
    fN = glm::normalize(fN);
    faceNormals[iF] = fN;
    faceAreas[iF] = fA;

    // Update incident vertices
    for (size_t j = 0; j < D; j++) {
      glm::vec3 pA = vertices[face[j]];
      glm::vec3 pB = vertices[face[(j + 1) % D]];
      glm::vec3 pC = vertices[face[(j + 2) % D]];

      vertexAreas[face[j]] += fA / D;

      // Corner angle for weighting normals
      double dot = glm::dot(glm::normalize(pB - pA), glm::normalize(pC - pA));
      float angle = std::acos(glm::clamp(-1., 1., dot));
      glm::vec3 normalContrib = angle * fN;

      if (std::isfinite(normalContrib.x) && std::isfinite(normalContrib.y) && std::isfinite(normalContrib.z)) {
        vertexNormals[face[(j + 1) % D]] += normalContrib;
      }

      // Compute edge lengths while we're at it
      edgeLengths[edgeIndices[iF][j]] = glm::length(pA - pB);
    }
  }


  // Normalize vertex normals
  for (auto& vec : vertexNormals) {
    double L = glm::length(vec);
    if (L > 0) {
      vec /= L;
    }
  }
  */
}


void VolumeMesh::draw() {
  if (!isEnabled()) {
    return;
  }

  render::engine->setBackfaceCull();

  // If no quantity is drawing the volume, we should draw it
  if (dominantQuantity == nullptr) {

    if (program == nullptr) {
      prepare();

      // do this now to reduce lag when picking later, etc
      preparePick();
    }

    // Set uniforms
    setStructureUniforms(*program);
    setVolumeMeshUniforms(*program);
    program->setUniform("u_baseColor1", getColor());
    program->setUniform("u_baseColor2", getInteriorColor());

    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
}

void VolumeMesh::drawPick() {
  if (!isEnabled()) {
    return;
  }

  if (pickProgram == nullptr) {
    preparePick();
  }

  // Set uniforms
  setStructureUniforms(*pickProgram);

  pickProgram->draw();
}

void VolumeMesh::prepare() {
  program = render::engine->requestShader("MESH", addVolumeMeshRules({"MESH_PROPAGATE_TYPE_AND_BASECOLOR2_SHADE"}));

  // Populate draw buffers
  fillGeometryBuffers(*program);
  render::engine->setMaterial(*program, getMaterial());
}

void VolumeMesh::preparePick() {

  // Create a new program
  pickProgram = render::engine->requestShader("MESH", addVolumeMeshRules({"MESH_PROPAGATE_PICK"}, false),
                                              render::ShaderReplacementDefaults::Pick);

  // == Sort out element counts and index ranges

  // TODO for now only picking cells and vertices

  // Get element indices
  size_t totalPickElements = nVertices() + nCells();

  // In "local" indices, indexing elements only within this mesh, used for reading later
  cellPickIndStart = nVertices();

  // In "global" indices, indexing all elements in the scene, used to fill buffers for drawing here
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);
  size_t cellGlobalPickIndStart = pickStart + nVertices();

  // == Fill buffers

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;
  std::vector<std::array<glm::vec3, 3>> vertexColors, edgeColors, halfedgeColors;
  std::vector<glm::vec3> faceColor;
  std::vector<glm::vec3> barycenters;

  bool wantsBarycenters = wantsCullPosition();

  // Reserve space
  // TODO
  positions.reserve(3 * nFacesTriangulation());
  bcoord.reserve(3 * nFacesTriangulation());
  vertexColors.reserve(3 * nFacesTriangulation());
  edgeColors.reserve(3 * nFacesTriangulation());
  halfedgeColors.reserve(3 * nFacesTriangulation());
  faceColor.reserve(3 * nFacesTriangulation());
  normals.reserve(3 * nFacesTriangulation());
  if (wantsBarycenters) {
    barycenters.reserve(3 * nFacesTriangulation());
  }


  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<int64_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    glm::vec3 cellColor = pick::indToVec(cellGlobalPickIndStart + iC);
    std::array<glm::vec3, 3> cellColorArr{cellColor, cellColor, cellColor};

    glm::vec3 barycenter;
    if (wantsBarycenters) {
      barycenter = cellCenter(iC);
    }

    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {

      // Do a first pass to compute a normal
      glm::vec3 normal{0., 0., 0.};
      for (const std::array<size_t, 3>& tri : face) {
        glm::vec3 pA = vertices[cell[tri[0]]];
        glm::vec3 pB = vertices[cell[tri[1]]];
        glm::vec3 pC = vertices[cell[tri[2]]];
        normal += glm::cross(pC - pB, pA - pB);
      }
      normal = glm::normalize(normal);


      // Emit the actual face triangulation
      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];

        std::array<glm::vec3, 3> vPos;
        std::array<glm::vec3, 3> vColor;
        for (int k = 0; k < 3; k++) {
          vPos[k] = vertices[cell[tri[k]]];
          vColor[k] = pick::indToVec(static_cast<size_t>(cell[tri[k]]) + pickStart);
        }

        for (int k = 0; k < 3; k++) {
          positions.push_back(vPos[k]);
          normals.push_back(normal);
          faceColor.push_back(cellColor);

          // need to pass each per-vertex value for these, since they will be interpolated
          vertexColors.push_back(vColor);
          edgeColors.push_back(cellColorArr);
          halfedgeColors.push_back(cellColorArr);
        }

        bcoord.push_back(glm::vec3{1., 0., 0.});
        bcoord.push_back(glm::vec3{0., 1., 0.});
        bcoord.push_back(glm::vec3{0., 0., 1.});

        if (wantsBarycenters) {
          for (int k = 0; k < 3; k++) {
            barycenters.push_back(barycenter);
          }
        }
      }
    }
  }


  // Store data in buffers
  pickProgram->setAttribute("a_position", positions);
  pickProgram->setAttribute("a_barycoord", bcoord);
  pickProgram->setAttribute("a_normal", normals);
  pickProgram->setAttribute<glm::vec3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_edgeColors", edgeColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_halfedgeColors", halfedgeColors);
  pickProgram->setAttribute("a_faceColor", faceColor);
  if (wantsCullPosition()) {
    pickProgram->setAttribute("a_cullPos", barycenters);
  }
}

std::vector<std::string> VolumeMesh::addVolumeMeshRules(std::vector<std::string> initRules, bool withSurfaceShade) {

  initRules = addStructureRules(initRules);

  if (withSurfaceShade) {
    if (getEdgeWidth() > 0) {
      initRules.push_back("MESH_WIREFRAME");
    }
  }

  initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");

  if (wantsCullPosition()) {
    initRules.push_back("MESH_PROPAGATE_CULLPOS");
  }

  return initRules;
}

void VolumeMesh::setVolumeMeshUniforms(render::ShaderProgram& p) {
  if (getEdgeWidth() > 0) {
    p.setUniform("u_edgeWidth", getEdgeWidth() * render::engine->getCurrentPixelScaling());
    p.setUniform("u_edgeColor", getEdgeColor());
  }
}

void VolumeMesh::fillGeometryBuffers(render::ShaderProgram& p) {

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;
  std::vector<glm::vec3> edgeReal;
  std::vector<double> faceTypes;
  std::vector<glm::vec3> barycenters;

  bool wantsBary = p.hasAttribute("a_barycoord");
  bool wantsEdge = (getEdgeWidth() > 0);
  bool wantsBarycenters = wantsCullPosition();
  bool wantsFaceType = p.hasAttribute("a_faceColorType");

  positions.reserve(3 * nFacesTriangulation());
  normals.reserve(3 * nFacesTriangulation());
  if (wantsBary) {
    bcoord.reserve(3 * nFacesTriangulation());
  }
  if (wantsEdge) {
    edgeReal.reserve(3 * nFacesTriangulation());
  }
  if (wantsBarycenters) {
    barycenters.reserve(3 * nFacesTriangulation());
  }
  if (wantsFaceType) {
    faceTypes.reserve(3 * nFacesTriangulation());
  }

  size_t iF = 0;
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<int64_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    glm::vec3 barycenter;
    if (wantsBarycenters) {
      barycenter = cellCenter(iC);
    }

    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {

      // Do a first pass to compute a normal
      glm::vec3 normal{0., 0., 0.};
      for (const std::array<size_t, 3>& tri : face) {
        glm::vec3 pA = vertices[cell[tri[0]]];
        glm::vec3 pB = vertices[cell[tri[1]]];
        glm::vec3 pC = vertices[cell[tri[2]]];
        normal += glm::cross(pC - pB, pA - pB);
      }
      normal = glm::normalize(normal);

      // Emit the actual face triangulation
      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];
        glm::vec3 pA = vertices[cell[tri[0]]];
        glm::vec3 pB = vertices[cell[tri[1]]];
        glm::vec3 pC = vertices[cell[tri[2]]];

        positions.push_back(pA);
        positions.push_back(pB);
        positions.push_back(pC);

        for (int k = 0; k < 3; k++) {
          normals.push_back(normal);
        }

        if (wantsFaceType) {
          float faceType = faceIsInterior[iF] ? 1. : 0.;
          for (int k = 0; k < 3; k++) {
            faceTypes.push_back(faceType);
          }
        }

        if (wantsBary) {
          bcoord.push_back(glm::vec3{1., 0., 0.});
          bcoord.push_back(glm::vec3{0., 1., 0.});
          bcoord.push_back(glm::vec3{0., 0., 1.});
        }

        if (wantsBarycenters) {
          for (int k = 0; k < 3; k++) {
            barycenters.push_back(barycenter);
          }
        }

        if (wantsEdge) {
          glm::vec3 edgeRealV{0., 1., 0.};
          if (j == 0) {
            edgeRealV.x = 1.;
          }
          if (j + 1 == face.size()) {
            edgeRealV.z = 1.;
          }
          for (int k = 0; k < 3; k++) {
            edgeReal.push_back(edgeRealV);
          }
        }
      }

      iF++;
    }
  }

  // Store data in buffers
  p.setAttribute("a_position", positions);
  p.setAttribute("a_normal", normals);
  if (wantsBary) {
    p.setAttribute("a_barycoord", bcoord);
  }
  if (wantsEdge) {
    p.setAttribute("a_edgeIsReal", edgeReal);
  }
  if (wantsCullPosition()) {
    p.setAttribute("a_cullPos", barycenters);
  }
  if (wantsFaceType) {
    p.setAttribute("a_faceColorType", faceTypes);
  }
}

const std::vector<std::vector<std::array<size_t, 3>>>& VolumeMesh::cellStencil(VolumeCellType type) {
  switch (type) {
  case VolumeCellType::TET:
    return stencilTet;
  case VolumeCellType::HEX:
    return stencilHex;
  }
}

glm::vec3 VolumeMesh::cellCenter(size_t iC) {

  glm::vec3 center{0., 0., 0};

  int count = 0;
  const std::array<int64_t, 8>& cell = cells[iC];
  for (int j = 0; j < 8; j++) {
    if (cell[j] >= 0) {
      center += vertices[cell[j]];
      count++;
    }
  }
  center /= count;

  return center;
}

void VolumeMesh::buildPickUI(size_t localPickID) {

  // Selection type
  if (localPickID < cellPickIndStart) {
    buildVertexInfoGui(localPickID);
  }
  // TODO faces and edges
  else {
    buildCellInfoGUI(localPickID - cellPickIndStart);
  }
}

void VolumeMesh::buildVertexInfoGui(size_t vInd) {

  size_t displayInd = vInd;
  if (vertexPerm.size() > 0) {
    displayInd = vertexPerm[vInd];
  }
  ImGui::TextUnformatted(("Vertex #" + std::to_string(displayInd)).c_str());

  std::stringstream buffer;
  buffer << vertices[vInd];
  ImGui::TextUnformatted(("Position: " + buffer.str()).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildVertexInfoGUI(vInd);
  }

  ImGui::Indent(-20.);
}
void VolumeMesh::buildFaceInfoGui(size_t fInd) {
  size_t displayInd = fInd;
  if (facePerm.size() > 0) {
    displayInd = facePerm[fInd];
  }
  ImGui::TextUnformatted(("Face #" + std::to_string(displayInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildFaceInfoGUI(fInd);
  }

  ImGui::Indent(-20.);
}

void VolumeMesh::buildEdgeInfoGui(size_t eInd) {
  size_t displayInd = eInd;
  if (edgePerm.size() > 0) {
    displayInd = edgePerm[eInd];
  }
  ImGui::TextUnformatted(("Edge #" + std::to_string(displayInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildEdgeInfoGUI(eInd);
  }

  ImGui::Indent(-20.);
}

void VolumeMesh::buildCellInfoGUI(size_t cellInd) {
  size_t displayInd = cellInd;
  // if (halfedgePerm.size() > 0) {
  // displayInd = halfedgePerm[cellInd];
  //}
  ImGui::TextUnformatted(("Cell #" + std::to_string(displayInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildCellInfoGUI(cellInd);
  }

  ImGui::Indent(-20.);
}


void VolumeMesh::buildCustomUI() {

  // Print stats
  long long int nVertsL = static_cast<long long int>(nVertices());
  long long int nCellsL = static_cast<long long int>(nCells());
  ImGui::Text("#verts: %lld  #cells: %lld", nVertsL, nCellsL);

  { // colors
    if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(color.get());
    ImGui::SameLine();

    if (ImGui::ColorEdit3("Interior", &interiorColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setInteriorColor(interiorColor.get());
    ImGui::SameLine();
  }

  ImGui::SameLine();
  { // Edge options
    ImGui::PushItemWidth(100);
    if (edgeWidth.get() == 0.) {
      bool showEdges = false;
      if (ImGui::Checkbox("Edges", &showEdges)) {
        setEdgeWidth(1.);
      }
    } else {
      bool showEdges = true;
      if (ImGui::Checkbox("Edges", &showEdges)) {
        setEdgeWidth(0.);
      }

      // Edge color
      ImGui::PushItemWidth(100);
      if (ImGui::ColorEdit3("Edge Color", &edgeColor.get()[0], ImGuiColorEditFlags_NoInputs))
        setEdgeColor(edgeColor.get());
      ImGui::PopItemWidth();

      // Edge width
      ImGui::SameLine();
      ImGui::PushItemWidth(60);
      if (ImGui::SliderFloat("Width", &edgeWidth.get(), 0.001, 2.)) {
        // NOTE: this intentionally circumvents the setEdgeWidth() setter to avoid repopulating the buffer as the
        // slider is dragged---otherwise we repopulate the buffer on every change, which mostly works fine. This is a
        // lazy solution instead of better state/buffer management. setEdgeWidth(getEdgeWidth());
        edgeWidth.manuallyChanged();
        requestRedraw();
      }
      ImGui::PopItemWidth();
    }
    ImGui::PopItemWidth();
  }
}

void VolumeMesh::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}


void VolumeMesh::refresh() {
  computeGeometryData();
  program.reset();
  pickProgram.reset();
  requestRedraw();
  QuantityStructure<VolumeMesh>::refresh(); // call base class version, which refreshes quantities
}

void VolumeMesh::geometryChanged() { refresh(); }

VolumeCellType VolumeMesh::cellType(size_t i) const {
  bool isTet = cells[i][4] < 0;
  if (isTet) return VolumeCellType::TET;
  return VolumeCellType::HEX;
};

double VolumeMesh::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (glm::vec3 p : vertices) {
    glm::vec3 transPos = glm::vec3(objectTransform.get() * glm::vec4(p.x, p.y, p.z, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(transPos - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> VolumeMesh::boundingBox() {
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3 pOrig : vertices) {
    glm::vec3 p = glm::vec3(objectTransform.get() * glm::vec4(pOrig, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

std::string VolumeMesh::typeName() { return structureTypeName; }


// === Option getters and setters

VolumeMesh* VolumeMesh::setColor(glm::vec3 val) {
  color = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeMesh::getColor() { return color.get(); }

VolumeMesh* VolumeMesh::setInteriorColor(glm::vec3 val) {
  interiorColor = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeMesh::getInteriorColor() { return interiorColor.get(); }


VolumeMesh* VolumeMesh::setEdgeColor(glm::vec3 val) {
  edgeColor = val;
  requestRedraw();
  return this;
}
glm::vec3 VolumeMesh::getEdgeColor() { return edgeColor.get(); }

VolumeMesh* VolumeMesh::setMaterial(std::string m) {
  material = m;
  refresh(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
  requestRedraw();
  return this;
}
std::string VolumeMesh::getMaterial() { return material.get(); }

VolumeMesh* VolumeMesh::setEdgeWidth(double newVal) {
  edgeWidth = newVal;
  refresh();
  requestRedraw();
  return this;
}
double VolumeMesh::getEdgeWidth() { return edgeWidth.get(); }


// === Quantity adders

VolumeMeshVertexColorQuantity* VolumeMesh::addVertexColorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& colors) {
  VolumeMeshVertexColorQuantity* q =
      new VolumeMeshVertexColorQuantity(name, applyPermutation(colors, vertexPerm), *this);
  addQuantity(q);
  return q;
}

VolumeMeshCellColorQuantity* VolumeMesh::addCellColorQuantityImpl(std::string name,
                                                                  const std::vector<glm::vec3>& colors) {
  VolumeMeshCellColorQuantity* q = new VolumeMeshCellColorQuantity(name, applyPermutation(colors, cellPerm), *this);
  addQuantity(q);
  return q;
}

VolumeMeshVertexScalarQuantity*
VolumeMesh::addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  VolumeMeshVertexScalarQuantity* q =
      new VolumeMeshVertexScalarQuantity(name, applyPermutation(data, vertexPerm), *this, type);
  addQuantity(q);
  return q;
}

VolumeMeshCellScalarQuantity* VolumeMesh::addCellScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                    DataType type) {
  VolumeMeshCellScalarQuantity* q =
      new VolumeMeshCellScalarQuantity(name, applyPermutation(data, facePerm), *this, type);
  addQuantity(q);
  return q;
}

/*

VolumeCornerParameterizationQuantity* VolumeMesh::addParameterizationQuantityImpl(std::string name,
                                                                                  const std::vector<glm::vec2>&
coords, ParamCoordsType type) { VolumeCornerParameterizationQuantity* q = new VolumeCornerParameterizationQuantity(
      name, applyPermutation(coords, cornerPerm), type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);

  return q;
}

VolumeVertexParameterizationQuantity*
VolumeMesh::addVertexParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                  ParamCoordsType type) {
  VolumeVertexParameterizationQuantity* q = new VolumeVertexParameterizationQuantity(
      name, applyPermutation(coords, vertexPerm), type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);

  return q;
}

VolumeVertexParameterizationQuantity*
VolumeMesh::addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                 ParamCoordsType type) {
  VolumeVertexParameterizationQuantity* q = new VolumeVertexParameterizationQuantity(
      name, applyPermutation(coords, vertexPerm), type, ParamVizStyle::LOCAL_CHECK, *this);
  addQuantity(q);

  return q;
}


VolumeVertexCountQuantity* VolumeMesh::addVertexCountQuantityImpl(std::string name,
                                                                  const std::vector<std::pair<size_t, int>>& values) {

  VolumeVertexCountQuantity* q = new VolumeVertexCountQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

VolumeVertexIsolatedScalarQuantity*
VolumeMesh::addVertexIsolatedScalarQuantityImpl(std::string name,
                                                const std::vector<std::pair<size_t, double>>& values) {
  VolumeVertexIsolatedScalarQuantity* q = new VolumeVertexIsolatedScalarQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

VolumeFaceCountQuantity* VolumeMesh::addFaceCountQuantityImpl(std::string name,
                                                              const std::vector<std::pair<size_t, int>>& values) {
  VolumeFaceCountQuantity* q = new VolumeFaceCountQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

VolumeGraphQuantity* VolumeMesh::addVolumeGraphQuantityImpl(std::string name, const std::vector<glm::vec3>& nodes,
                                                            const std::vector<std::array<size_t, 2>>& edges) {
  VolumeGraphQuantity* q = new VolumeGraphQuantity(name, nodes, edges, *this);
  addQuantity(q);
  return q;
}




VolumeEdgeScalarQuantity* VolumeMesh::addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                DataType type) {
  VolumeEdgeScalarQuantity* q = new VolumeEdgeScalarQuantity(name, applyPermutation(data, edgePerm), *this, type);
  addQuantity(q);
  return q;
}

VolumeHalfedgeScalarQuantity*
VolumeMesh::addHalfedgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  VolumeHalfedgeScalarQuantity* q =
      new VolumeHalfedgeScalarQuantity(name, applyPermutation(data, halfedgePerm), *this, type);
  addQuantity(q);
  return q;
}


VolumeVertexVectorQuantity* VolumeMesh::addVertexVectorQuantityImpl(std::string name,
                                                                    const std::vector<glm::vec3>& vectors,
                                                                    VectorType vectorType) {
  VolumeVertexVectorQuantity* q =
      new VolumeVertexVectorQuantity(name, applyPermutation(vectors, vertexPerm), *this, vectorType);
  addQuantity(q);
  return q;
}

VolumeFaceVectorQuantity* VolumeMesh::addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>&
vectors, VectorType vectorType) {

  VolumeFaceVectorQuantity* q =
      new VolumeFaceVectorQuantity(name, applyPermutation(vectors, facePerm), *this, vectorType);
  addQuantity(q);
  return q;
}

*/

VolumeMeshQuantity::VolumeMeshQuantity(std::string name, VolumeMesh& parentStructure, bool dominates)
    : Quantity<VolumeMesh>(name, parentStructure, dominates) {}
void VolumeMeshQuantity::buildVertexInfoGUI(size_t vInd) {}
void VolumeMeshQuantity::buildFaceInfoGUI(size_t fInd) {}
void VolumeMeshQuantity::buildEdgeInfoGUI(size_t eInd) {}
void VolumeMeshQuantity::buildCellInfoGUI(size_t cInd) {}

} // namespace polyscope

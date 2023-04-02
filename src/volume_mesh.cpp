// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_mesh.h"

#include "polyscope/color_management.h"
#include "polyscope/combining_hash_functions.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"
#include "polyscope/volume_mesh_quantity.h"

#include "imgui.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <utility>

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

// Indirection to place vertex 0 always in the bottom left corner
const std::array<std::array<size_t, 8>, 8> VolumeMesh::rotationMap = 
 {{
   {0, 1, 2, 3, 4, 5, 7, 6}, 
   {1, 0, 4, 5, 2, 3, 6, 7}, 
   {2, 1, 5, 6, 3, 0, 7, 4}, 
   {3, 0, 1, 2, 7, 4, 6, 5}, 
   {4, 0, 3, 7, 5, 1, 6, 2}, 
   {5, 1, 0, 4, 7, 2, 6, 3}, 
   {7, 3, 2, 6, 4, 0, 5, 1},
   {6, 2, 1, 5, 7, 3, 4, 0} 
 }};

// Map indirected cube to tets
const std::array<std::array<std::array<size_t, 4>, 6>, 4> VolumeMesh::diagonalMap = 
 {{
    {{
      {0, 1, 2, 5},
      {0, 2, 6, 5},
      {0, 2, 3, 6},
      {0, 5, 6, 4},
      {2, 6, 5, 7},
      {0, 0, 0, 0}
    }},
    {{
      {0, 5, 6, 4},
      {0, 1, 6, 5},
      {1, 7, 6, 5},
      {0, 6, 2, 3},
      {0, 6, 1, 2},
      {1, 6, 7, 2}
    }},
    {{
      {0, 4, 5, 7},
      {0, 3, 6, 7},
      {0, 6, 4, 7},
      {0, 1, 2, 5},
      {0, 3, 7, 2},
      {0, 7, 5, 2}
    }},
    {{
      {0, 2, 3, 7},
      {0, 3, 6, 7},
      {0, 6, 4, 7},
      {0, 5, 7, 4},
      {1, 5, 7, 0},
      {1, 7, 2, 0}
    }}
 }};


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

VolumeMesh::VolumeMesh(std::string name, const std::vector<glm::vec3>& vertexPositions_,
                       const std::vector<std::array<uint32_t, 8>>& cellIndices_)
    : QuantityStructure<VolumeMesh>(name, typeName()),
      // clang-format off

// == managed quantities

// positions
vertexPositions(        uniquePrefix() + "vertexPositions",     vertexPositionsData),

// connectivity / indices
triangleVertexInds(     uniquePrefix() + "triangleVertexInds",  triangleVertexIndsData),
triangleFaceInds(       uniquePrefix() + "triangleFaceInds",    triangleFaceIndsData),
triangleCellInds(       uniquePrefix() + "triangleCellInds",    triangleCellIndsData),

// internal triangle data for rendering
baryCoord(              uniquePrefix() + "baryCoord",           baryCoordData),
edgeIsReal(             uniquePrefix() + "edgeIsReal",          edgeIsRealData),
faceType(               uniquePrefix() + "faceType",            faceTypeData),

// other internally-computed geometry
faceNormals(            uniquePrefix() + "faceNormals",         faceNormalsData,        std::bind(&VolumeMesh::computeFaceNormals, this)),
cellCenters(            uniquePrefix() + "cellCenters",         cellCentersData,        std::bind(&VolumeMesh::computeCellCenters, this)),         


// == core input data
cells(cellIndices_),
vertexPositionsData(vertexPositions_), 

// == persistent options
color(uniquePrefix() + "color", getNextUniqueColor()),
interiorColor(uniquePrefix() + "interiorColor", color.get()),
edgeColor(uniquePrefix() + "edgeColor", glm::vec3{0., 0., 0.}), 
material(uniquePrefix() + "material", "clay"),
edgeWidth(uniquePrefix() + "edgeWidth", 0.), 

// == misc values
activeLevelSetQuantity(nullptr) 
{
  // clang-format on

  cullWholeElements.setPassive(true);

  // set the interior color to be a desaturated version of the normal one
  glm::vec3 desatColorHSV = RGBtoHSV(color.get());
  desatColorHSV.y *= 0.3;
  interiorColor.setPassive(HSVtoRGB(desatColorHSV));

  computeCounts();
  computeConnectivityData();
  updateObjectSpaceBounds();
}

void VolumeMesh::computeCounts() {

  // == Populate counts
  nFacesCount = 0;
  nFacesTriangulationCount = 0;

  // == Populate interior/exterior faces
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);
    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      nFacesCount++;
      nFacesTriangulationCount += face.size();
    }
  }

  // == Step 1: count occurences of each face
  std::unordered_map<std::array<uint32_t, 4>, int, polyscope::hash_combine::hash<std::array<uint32_t, 4>>> faceCounts;

  std::set<size_t> faceInds; // Scratch map

  // Build a sorted list of the indices of this face

  // Helper to build a sorted-index array for a face
  auto generateSortedFace = [&](const std::array<uint32_t, 8>& cell, const std::vector<std::array<size_t, 3>>& face) {
    faceInds.clear();
    for (const std::array<size_t, 3>& tri : face) {
      for (int j = 0; j < 3; j++) {
        faceInds.insert(cell[tri[j]]);
      }
    }
    std::array<uint32_t, 4> sortedFace{7777, 7777, 7777, 7777};
    int j = 0;
    for (size_t ind : faceInds) {
      sortedFace[j] = ind;
      j++;
    }
    return sortedFace;
  };

  // Iterate over cells
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      std::array<uint32_t, 4> sortedFace = generateSortedFace(cell, face);
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
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    // Iterate over faces
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {
      std::array<uint32_t, 4> sortedFace = generateSortedFace(cell, face);
      faceIsInterior.push_back(faceCounts[sortedFace] > 1);
    }
  }
}


void VolumeMesh::computeTets() {
  // Algorithm from
  // https://www.researchgate.net/profile/Julien-Dompierre/publication/221561839_How_to_Subdivide_Pyramids_Prisms_and_Hexahedra_into_Tetrahedra/links/0912f509c0b7294059000000/How-to-Subdivide-Pyramids-Prisms-and-Hexahedra-into-Tetrahedra.pdf?origin=publication_detail
  // It's a bit hard to look at but it works
  // Uses vertex numberings to ensure consistent diagonals between faces, and keeps tet counts to 5 or 6 per hex
  size_t tetCount = 0;
  // Get number of tets first
  for (size_t iC = 0; iC < nCells(); iC++) {
    switch (cellType(iC)) {
    case VolumeCellType::HEX: {
      std::array<size_t, 8> sortedNumbering;
      std::iota(sortedNumbering.begin(), sortedNumbering.end(), 0);
      std::sort(sortedNumbering.begin(), sortedNumbering.end(),
                [this, iC](size_t a, size_t b) -> bool { return cells[iC][a] < cells[iC][b]; });
      std::array<size_t, 8> rotatedNumbering;
      std::copy(rotationMap[sortedNumbering[0]].begin(), rotationMap[sortedNumbering[0]].end(),
                rotatedNumbering.begin());
      size_t diagCount = 0;
      auto checkDiagonal = [this, rotatedNumbering, iC](size_t a1, size_t a2, size_t b1, size_t b2) {
        return (cells[iC][rotatedNumbering[a1]] < cells[iC][rotatedNumbering[b1]] &&
                cells[iC][rotatedNumbering[a1]] < cells[iC][rotatedNumbering[b2]]) ||
               (cells[iC][rotatedNumbering[a2]] < cells[iC][rotatedNumbering[b1]] &&
                cells[iC][rotatedNumbering[a2]] < cells[iC][rotatedNumbering[b2]]);
      };
      if (checkDiagonal(1, 7, 2, 5)) {
        diagCount++;
      }
      if (checkDiagonal(3, 7, 2, 6)) {
        diagCount++;
      }
      if (checkDiagonal(4, 7, 5, 6)) {
        diagCount++;
      }
      if (diagCount == 0) {
        tetCount += 5;
      } else {
        tetCount += 6;
      }
      break;
    }
    case VolumeCellType::TET:
      tetCount += 1;
      break;
    }
  }
  // Mark each edge as real or not (in the original mesh)
  std::vector<std::array<bool, 6>> realEdges;
  // Each hex can make up to 6 tets
  tets.resize(tetCount);
  realEdges.resize(tetCount);
  size_t tetIdx = 0;
  for (size_t iC = 0; iC < nCells(); iC++) {
    switch (cellType(iC)) {
    case VolumeCellType::HEX: {
      std::array<size_t, 8> sortedNumbering;
      std::iota(sortedNumbering.begin(), sortedNumbering.end(), 0);
      std::sort(sortedNumbering.begin(), sortedNumbering.end(),
                [this, iC](size_t a, size_t b) -> bool { return cells[iC][a] < cells[iC][b]; });
      std::array<size_t, 8> rotatedNumbering;
      std::copy(rotationMap[sortedNumbering[0]].begin(), rotationMap[sortedNumbering[0]].end(),
                rotatedNumbering.begin());
      size_t n = 0;
      size_t diagCount = 0;
      // Diagonal exists on the pair of vertices which contain the minimum vertex number
      auto checkDiagonal = [this, rotatedNumbering, iC](size_t a1, size_t a2, size_t b1, size_t b2) {
        return (cells[iC][rotatedNumbering[a1]] < cells[iC][rotatedNumbering[b1]] &&
                cells[iC][rotatedNumbering[a1]] < cells[iC][rotatedNumbering[b2]]) ||
               (cells[iC][rotatedNumbering[a2]] < cells[iC][rotatedNumbering[b1]] &&
                cells[iC][rotatedNumbering[a2]] < cells[iC][rotatedNumbering[b2]]);
      };
      // Minimum vertex will always have 3 diagonals, check other three faces
      if (checkDiagonal(1, 7, 2, 5)) {
        n += 4;
        diagCount++;
      }
      if (checkDiagonal(3, 7, 2, 6)) {
        n += 2;
        diagCount++;
      }
      if (checkDiagonal(4, 7, 5, 6)) {
        n += 1;
        diagCount++;
      }
      // Rotate by 120 or 240 degrees depending on diagonal positions
      if (n == 1 || n == 6) {
        size_t temp = rotatedNumbering[1];
        rotatedNumbering[1] = rotatedNumbering[4];
        rotatedNumbering[4] = rotatedNumbering[3];
        rotatedNumbering[3] = temp;
        temp = rotatedNumbering[5];
        rotatedNumbering[5] = rotatedNumbering[6];
        rotatedNumbering[6] = rotatedNumbering[2];
        rotatedNumbering[2] = temp;
      } else if (n == 2 || n == 5) {
        size_t temp = rotatedNumbering[1];
        rotatedNumbering[1] = rotatedNumbering[3];
        rotatedNumbering[3] = rotatedNumbering[4];
        rotatedNumbering[4] = temp;
        temp = rotatedNumbering[5];
        rotatedNumbering[5] = rotatedNumbering[2];
        rotatedNumbering[2] = rotatedNumbering[6];
        rotatedNumbering[6] = temp;
      }

      // Map final tets according to diagonalMap and the number of diagonals not incident to V_0
      std::array<std::array<size_t, 4>, 6> tetMap = diagonalMap[diagCount];
      for (size_t k = 0; k < (diagCount == 0 ? 5 : 6); k++) {
        for (size_t i = 0; i < 4; i++) {
          tets[tetIdx][i] = cells[iC][rotatedNumbering[tetMap[k][i]]];
        }
        tetIdx++;
      }
      break;
    }
    case VolumeCellType::TET:
      for (size_t i = 0; i < 4; i++) {
        tets[tetIdx][i] = cells[iC][i];
      }
      tetIdx++;
      break;
    }
  }
}

void VolumeMesh::ensureHaveTets() {
  if (tets.empty()) {
    computeTets();
  }
}

size_t VolumeMesh::nTets() {
  ensureHaveTets();
  return tets.size();
}

void VolumeMesh::addSlicePlaneListener(polyscope::SlicePlane* sp) { volumeSlicePlaneListeners.push_back(sp); }

void VolumeMesh::removeSlicePlaneListener(polyscope::SlicePlane* sp) {
  for (size_t i = 0; i < volumeSlicePlaneListeners.size(); i++) {
    if (volumeSlicePlaneListeners[i] == sp) {
      volumeSlicePlaneListeners.erase(volumeSlicePlaneListeners.begin() + i);
      break;
    }
  }
}

void VolumeMesh::fillSliceGeometryBuffers(render::ShaderProgram& program) {

  // TODO update this to use new standalone buffers

  ensureHaveTets();
  vertexPositions.ensureHostBufferPopulated();

  // TODO port this to managed buffers

  std::vector<glm::vec3> point1;
  std::vector<glm::vec3> point2;
  std::vector<glm::vec3> point3;
  std::vector<glm::vec3> point4;
  size_t tetCount = tets.size();
  point1.resize(tetCount);
  point2.resize(tetCount);
  point3.resize(tetCount);
  point4.resize(tetCount);
  for (size_t tetIdx = 0; tetIdx < tets.size(); tetIdx++) {
    point1[tetIdx] = vertexPositions.data[tets[tetIdx][0]];
    point2[tetIdx] = vertexPositions.data[tets[tetIdx][1]];
    point3[tetIdx] = vertexPositions.data[tets[tetIdx][2]];
    point4[tetIdx] = vertexPositions.data[tets[tetIdx][3]];
  }

  program.setAttribute("a_point_1", point1);
  program.setAttribute("a_point_2", point2);
  program.setAttribute("a_point_3", point3);
  program.setAttribute("a_point_4", point4);
  program.setAttribute("a_slice_1", point1);
  program.setAttribute("a_slice_2", point2);
  program.setAttribute("a_slice_3", point3);
  program.setAttribute("a_slice_4", point4);
}


VolumeMeshVertexScalarQuantity* VolumeMesh::getLevelSetQuantity() { return activeLevelSetQuantity; }

void VolumeMesh::setLevelSetQuantity(VolumeMeshVertexScalarQuantity* quantity) {
  if (activeLevelSetQuantity != nullptr && activeLevelSetQuantity != quantity) {
    activeLevelSetQuantity->isDrawingLevelSet = false;
  }
  activeLevelSetQuantity = quantity;
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
    glm::mat4 viewMat = getModelView();
    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    program->setUniform("u_baseColor1", getColor());
    program->setUniform("u_baseColor2", getInteriorColor());

    program->draw();
  }

  if (activeLevelSetQuantity != nullptr && activeLevelSetQuantity->isEnabled()) {
    // Draw the quantities
    activeLevelSetQuantity->draw();

    return;
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void VolumeMesh::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
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
  setVolumeMeshUniforms(*pickProgram);
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
  pickProgram = render::engine->requestShader("MESH", addVolumeMeshRules({"MESH_PROPAGATE_PICK_SIMPLE"}),
                                              render::ShaderReplacementDefaults::Pick);

  fillGeometryBuffers(*pickProgram);

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

  std::vector<std::array<glm::vec3, 3>> vertexColors, edgeColors, halfedgeColors, cornerColors;
  std::vector<glm::vec3> faceColor;

  // Reserve space
  vertexColors.resize(3 * nFacesTriangulation());
  edgeColors.resize(3 * nFacesTriangulation());
  halfedgeColors.resize(3 * nFacesTriangulation());
  cornerColors.resize(3 * nFacesTriangulation());
  faceColor.resize(3 * nFacesTriangulation());

  size_t iFront = 0;
  size_t iBack = nFacesTriangulation() - 1;
  size_t iF = 0;
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    glm::vec3 cellColor = pick::indToVec(cellGlobalPickIndStart + iC);
    std::array<glm::vec3, 3> cellColorArr{cellColor, cellColor, cellColor};

    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {

      // Emit the actual face in the triangulation
      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];

        std::array<glm::vec3, 3> vColor;
        for (int k = 0; k < 3; k++) {
          vColor[k] = pick::indToVec(static_cast<size_t>(cell[tri[k]]) + pickStart);
        }

        // Push exterior faces to the front of the draw buffer, and interior faces to the back.
        // (see note above)
        size_t iData;
        if (faceIsInterior[iF]) {
          iData = iBack;
          iBack--;
        } else {
          iData = iFront;
          iFront++;
        }

        for (int k = 0; k < 3; k++) faceColor[3 * iData + k] = cellColor;
        for (int k = 0; k < 3; k++) vertexColors[3 * iData + k] = vColor;
      }

      iF++;
    }
  }


  // Store data in buffers
  pickProgram->setAttribute<glm::vec3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute("a_faceColor", faceColor);
}

std::vector<std::string> VolumeMesh::addVolumeMeshRules(std::vector<std::string> initRules, bool withSurfaceShade,
                                                        bool isSlice) {

  initRules = addStructureRules(initRules);

  if (withSurfaceShade) {
    if (getEdgeWidth() > 0) {
      initRules.push_back(isSlice ? "SLICE_TETS_MESH_WIREFRAME" : "MESH_WIREFRAME");
    }
  }

  initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");

  if (wantsCullPosition() && !isSlice) {
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

  p.setAttribute("a_vertexPositions", vertexPositions.getIndexedRenderAttributeBuffer(triangleVertexInds));

  p.setAttribute("a_vertexNormals", faceNormals.getIndexedRenderAttributeBuffer(triangleFaceInds));

  bool wantsBary = p.hasAttribute("a_barycoord");
  bool wantsEdge = (getEdgeWidth() > 0);
  bool wantsAttrCullPosition = wantsCullPosition();
  bool wantsFaceType = p.hasAttribute("a_faceColorType");

  if (wantsBary) {
    p.setAttribute("a_barycoord", baryCoord.getRenderAttributeBuffer());
  }
  if (wantsEdge) {
    p.setAttribute("a_edgeIsReal", edgeIsReal.getRenderAttributeBuffer());
  }
  if (wantsAttrCullPosition) {
    p.setAttribute("a_cullPos", cellCenters.getIndexedRenderAttributeBuffer(triangleCellInds));
  }
  if (wantsFaceType) {
    p.setAttribute("a_faceColorType", faceType.getIndexedRenderAttributeBuffer(triangleFaceInds));
  }
}

void VolumeMesh::computeConnectivityData() {

  // NOTE: If we were to fill buffers naively via a loop over cells, we get pretty bad z-fighting artifacts where
  // interior edges ever-so-slightly show through the exterior boundary (more generally, any place 3 faces meet at an
  // edge, which happens everywhere in a tet mesh).
  //
  // To mitigate this issue, we fill the buffer such that all exterior faces come first, then all interior faces, so
  // that exterior faces always win depth ties. This doesn't totally eliminate the problem, but greatly improves the
  // most egregious cases.

  // == Allocate buffers
  triangleVertexInds.data.clear();
  triangleVertexInds.data.resize(3 * nFacesTriangulation());
  triangleFaceInds.data.clear();
  triangleFaceInds.data.resize(3 * nFacesTriangulation());
  triangleCellInds.data.clear();
  triangleCellInds.data.resize(3 * nFacesTriangulation());
  triangleCellInds.data.clear();
  triangleCellInds.data.resize(3 * nFacesTriangulation());
  baryCoord.data.clear();
  baryCoord.data.resize(3 * nFacesTriangulation());
  edgeIsReal.data.clear();
  edgeIsReal.data.resize(3 * nFacesTriangulation());
  faceType.data.clear();
  faceType.data.resize(nFaces());

  size_t iF = 0;
  size_t iFront = 0;
  size_t iBack = nFacesTriangulation() - 1;
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    // Loop over all faces of the cell
    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {

      // Loop over the face's triangulation
      for (size_t j = 0; j < face.size(); j++) {
        const std::array<size_t, 3>& tri = face[j];

        // Enumerate exterior faces in the front of the draw buffer, and interior faces in the back.
        // (see note above)
        size_t iData;
        if (faceIsInterior[iF]) {
          iData = iBack;
          iBack--;
        } else {
          iData = iFront;
          iFront++;
        }

        for (size_t k = 0; k < 3; k++) {
          triangleVertexInds.data[3 * iData + k] = cell[tri[k]];
        }
        for (size_t k = 0; k < 3; k++) triangleFaceInds.data[3 * iData + k] = iF;
        for (size_t k = 0; k < 3; k++) triangleCellInds.data[3 * iData + k] = iC;

        baryCoord.data[3 * iData + 0] = glm::vec3{1., 0., 0.};
        baryCoord.data[3 * iData + 1] = glm::vec3{0., 1., 0.};
        baryCoord.data[3 * iData + 2] = glm::vec3{0., 0., 1.};

        glm::vec3 edgeRealV{0., 1., 0.};
        if (j == 0) edgeRealV.x = 1.;
        if (j + 1 == face.size()) edgeRealV.z = 1.;
        for (int k = 0; k < 3; k++) edgeIsReal.data[3 * iData + k] = edgeRealV;
      }

      float faceTypeFloat = faceIsInterior[iF] ? 1. : 0.;
      for (int k = 0; k < 3; k++) faceType.data[iF] = faceTypeFloat;

      iF++;
    }
  }

  triangleVertexInds.markHostBufferUpdated();
  triangleFaceInds.markHostBufferUpdated();
  triangleCellInds.markHostBufferUpdated();
  triangleCellInds.markHostBufferUpdated();
  baryCoord.markHostBufferUpdated();
  edgeIsReal.markHostBufferUpdated();
  faceType.markHostBufferUpdated();
}

const std::vector<std::vector<std::array<size_t, 3>>>& VolumeMesh::cellStencil(VolumeCellType type) {
  switch (type) {
  case VolumeCellType::TET:
    return stencilTet;
  case VolumeCellType::HEX:
    return stencilHex;
  }

  // unreachable
  return stencilTet;
}


void VolumeMesh::computeFaceNormals() {

  vertexPositions.ensureHostBufferPopulated();

  faceNormals.data.resize(nFaces());

  size_t iF = 0;
  for (size_t iC = 0; iC < nCells(); iC++) {
    const std::array<uint32_t, 8>& cell = cells[iC];
    VolumeCellType cellT = cellType(iC);

    for (const std::vector<std::array<size_t, 3>>& face : cellStencil(cellT)) {

      // Do a first pass to compute a normal
      glm::vec3 normal{0., 0., 0.};
      for (const std::array<size_t, 3>& tri : face) {
        glm::vec3 pA = vertexPositions.data[cell[tri[0]]];
        glm::vec3 pB = vertexPositions.data[cell[tri[1]]];
        glm::vec3 pC = vertexPositions.data[cell[tri[2]]];
        normal += glm::cross(pC - pB, pA - pB);
      }
      normal = glm::normalize(normal);

      faceNormals.data[iF] = normal;
      iF++;
    }
  }

  faceNormals.markHostBufferUpdated();
}


void VolumeMesh::computeCellCenters() {

  vertexPositions.ensureHostBufferPopulated();

  cellCenters.data.resize(nCells());

  for (size_t iC = 0; iC < nCells(); iC++) {

    glm::vec3 center{0., 0., 0};

    int count = 0;
    const std::array<uint32_t, 8>& cell = cells[iC];
    for (int j = 0; j < 8; j++) {
      if (cell[j] < INVALID_IND_32) {
        center += vertexPositions.data[cell[j]];
        count++;
      }
    }
    center /= count;

    cellCenters.data[iC] = center;
  }

  cellCenters.markHostBufferUpdated();
}

void VolumeMesh::buildPickUI(size_t localPickID) {

  // Selection type
  if (localPickID < cellPickIndStart) {
    buildVertexInfoGui(localPickID);
  } else {
    buildCellInfoGUI(localPickID - cellPickIndStart);
  }
}

void VolumeMesh::buildVertexInfoGui(size_t vInd) {

  size_t displayInd = vInd;
  ImGui::TextUnformatted(("Vertex #" + std::to_string(displayInd)).c_str());

  std::stringstream buffer;
  buffer << vertexPositions.getValue(vInd);
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

void VolumeMesh::buildCellInfoGUI(size_t cellInd) {
  size_t displayInd = cellInd;
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


void VolumeMesh::refreshVolumeMeshListeners() {
  for (size_t i = 0; i < volumeSlicePlaneListeners.size(); i++) {
    volumeSlicePlaneListeners[i]->resetVolumeSliceProgram();
  }
}

void VolumeMesh::refresh() {
  program.reset();
  pickProgram.reset();
  refreshVolumeMeshListeners();
  requestRedraw();
  QuantityStructure<VolumeMesh>::refresh(); // call base class version, which refreshes quantities
}

void VolumeMesh::geometryChanged() {
  recomputeGeometryIfPopulated();
  requestRedraw();
  QuantityStructure<VolumeMesh>::refresh();
}

void VolumeMesh::recomputeGeometryIfPopulated() {
  faceNormals.recomputeIfPopulated();
  cellCenters.recomputeIfPopulated();
}

VolumeCellType VolumeMesh::cellType(size_t i) const {
  bool isHex = cells[i][4] < INVALID_IND_32;
  if (isHex) {
    return VolumeCellType::HEX;
  } else {
    return VolumeCellType::TET;
  }
};

void VolumeMesh::updateObjectSpaceBounds() {

  vertexPositions.ensureHostBufferPopulated();

  // bounding box
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  for (const glm::vec3& p : vertexPositions.data) {
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }
  objectSpaceBoundingBox = std::make_tuple(min, max);

  // length scale, as twice the radius from the center of the bounding box
  glm::vec3 center = 0.5f * (min + max);
  float lengthScale = 0.0;
  for (const glm::vec3& p : vertexPositions.data) {
    lengthScale = std::max(lengthScale, glm::length2(p - center));
  }
  objectSpaceLengthScale = 2 * std::sqrt(lengthScale);
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


// === Quantity adder}

VolumeMeshVertexColorQuantity* VolumeMesh::addVertexColorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& colors) {
  VolumeMeshVertexColorQuantity* q = new VolumeMeshVertexColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

VolumeMeshCellColorQuantity* VolumeMesh::addCellColorQuantityImpl(std::string name,
                                                                  const std::vector<glm::vec3>& colors) {
  VolumeMeshCellColorQuantity* q = new VolumeMeshCellColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

VolumeMeshVertexScalarQuantity*
VolumeMesh::addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  VolumeMeshVertexScalarQuantity* q = new VolumeMeshVertexScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

VolumeMeshCellScalarQuantity* VolumeMesh::addCellScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                    DataType type) {
  VolumeMeshCellScalarQuantity* q = new VolumeMeshCellScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

VolumeMeshVertexVectorQuantity* VolumeMesh::addVertexVectorQuantityImpl(std::string name,
                                                                        const std::vector<glm::vec3>& vectors,
                                                                        VectorType vectorType) {
  VolumeMeshVertexVectorQuantity* q = new VolumeMeshVertexVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

VolumeMeshCellVectorQuantity*
VolumeMesh::addCellVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType) {

  VolumeMeshCellVectorQuantity* q = new VolumeMeshCellVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}


VolumeMeshQuantity::VolumeMeshQuantity(std::string name, VolumeMesh& parentStructure, bool dominates)
    : QuantityS<VolumeMesh>(name, parentStructure, dominates) {}
void VolumeMeshQuantity::buildVertexInfoGUI(size_t vInd) {}
void VolumeMeshQuantity::buildFaceInfoGUI(size_t fInd) {}
void VolumeMeshQuantity::buildEdgeInfoGUI(size_t eInd) {}
void VolumeMeshQuantity::buildCellInfoGUI(size_t cInd) {}

} // namespace polyscope

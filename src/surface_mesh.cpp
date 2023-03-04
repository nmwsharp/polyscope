// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_mesh.h"

#include "glm/fwd.hpp"
#include "polyscope/combining_hash_functions.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"
#include "polyscope/types.h"
#include "polyscope/utilities.h"

#include <unordered_map>
#include <utility>

namespace polyscope {

// Initialize statics
const std::string SurfaceMesh::structureTypeName = "Surface Mesh";


SurfaceMesh::SurfaceMesh(std::string name_)
    : QuantityStructure<SurfaceMesh>(name_, typeName()),
      // clang-format off

// = geometric quantities

// positions
vertexPositions(        uniquePrefix() + "vertexPositions",     vertexPositionsData),

// connectivity / indices
// (triangle and face inds are always computed initially when we triangulate the mesh)
triangleVertexInds(     uniquePrefix() + "triangleVertexInds",  triangleVertexIndsData),
triangleFaceInds(       uniquePrefix() + "triangleFaceInds",    triangleFaceIndsData),
triangleEdgeInds(       uniquePrefix() + "triangleEdgeInds",    triangleEdgeIndsData,       std::bind(&SurfaceMesh::computeTriangleEdgeInds, this)),
triangleHalfedgeInds(   uniquePrefix() + "triangleHalfedgeInds",triangleHalfedgeIndsData,   std::bind(&SurfaceMesh::computeTriangleHalfedgeInds, this)),
triangleCornerInds(     uniquePrefix() + "triangleCornerInds",  triangleCornerIndsData,     std::bind(&SurfaceMesh::computeTriangleCornerInds, this)),

// internal triangle data for rendering
baryCoord(              uniquePrefix() + "baryCoord",           baryCoordData),
edgeIsReal(             uniquePrefix() + "edgeIsReal",          edgeIsRealData),

// other internally-computed geometry
faceNormals(            uniquePrefix() + "faceNormals",         faceNormalsData,        std::bind(&SurfaceMesh::computeFaceNormals, this)),
faceCenters(            uniquePrefix() + "faceCenters",         faceCentersData,        std::bind(&SurfaceMesh::computeFaceCenters, this)),         
faceAreas(              uniquePrefix() + "faceAreas",           faceAreasData,          std::bind(&SurfaceMesh::computeFaceAreas, this)),
vertexNormals(          uniquePrefix() + "vertexNormals",       vertexNormalsData,      std::bind(&SurfaceMesh::computeVertexNormals, this)),
vertexAreas(            uniquePrefix() + "vertexAreas",         vertexAreasData,        std::bind(&SurfaceMesh::computeVertexAreas, this)),

// tangent spaces
faceTangentSpaces(          uniquePrefix() + "faceTangentSpaces",   faceTangentSpacesData),
vertexTangentSpaces(        uniquePrefix() + "vertexTangentSpace",  vertexTangentSpacesData),
defaultFaceTangentSpaces(   uniquePrefix() + "defaultFaceTangentSpace",  defaultFaceTangentSpacesData,  std::bind(&SurfaceMesh::computeDefaultFaceTangentSpaces, this)),

// = persistent options
surfaceColor(           uniquePrefix() + "surfaceColor",    getNextUniqueColor()),
edgeColor(              uniquePrefix() + "edgeColor",       glm::vec3{0., 0., 0.}), material(uniquePrefix() + "material", "clay"),
edgeWidth(              uniquePrefix() + "edgeWidth",       0.),
backFacePolicy(         uniquePrefix() + "backFacePolicy",  BackFacePolicy::Different),
backFaceColor(          uniquePrefix() + "backFaceColor",   glm::vec3(1.f - surfaceColor.get().r, 1.f - surfaceColor.get().g, 1.f - surfaceColor.get().b)),
shadeStyle(             uniquePrefix() + "shadeStyle",      MeshShadeStyle::Flat)

// clang-format on
{}

SurfaceMesh::SurfaceMesh(std::string name_, const std::vector<glm::vec3>& vertexPositions_,
                         const std::vector<uint32_t>& faceIndsEntries_, const std::vector<uint32_t>& faceIndsStart_)
    : SurfaceMesh(name_) {

  vertexPositionsData = vertexPositions_;
  faceIndsEntries = faceIndsEntries_;
  faceIndsStart = faceIndsStart_;

  computeConnectivityData();
  updateObjectSpaceBounds();
}

SurfaceMesh::SurfaceMesh(std::string name_, const std::vector<glm::vec3>& vertexPositions_,
                         const std::vector<std::vector<size_t>>& facesIn)
    : SurfaceMesh(name_) {

  vertexPositionsData = vertexPositions_;
  nestedFacesToFlat(facesIn);

  computeConnectivityData();
  updateObjectSpaceBounds();
}

void SurfaceMesh::nestedFacesToFlat(const std::vector<std::vector<size_t>>& nestedInds) {

  faceIndsStart.clear();
  faceIndsEntries.clear();
  faceIndsStart.push_back(0);

  for (const std::vector<size_t>& face : nestedInds) {
    for (size_t iV : face) {
      faceIndsEntries.push_back(iV);
    }
    faceIndsStart.push_back(faceIndsEntries.size());
  }
}

void SurfaceMesh::computeConnectivityData() {

  // some number-of-elements arithmetic
  size_t numFaces = faceIndsStart.size() - 1;
  nCornersCount = faceIndsEntries.size();
  nFacesTriangulationCount = nCornersCount - 2 * numFaces;

  // fill out these buffers as we construct the triangulation
  triangleVertexIndsData.clear();
  triangleVertexIndsData.resize(3 * nFacesTriangulationCount);
  triangleFaceIndsData.clear();
  triangleFaceIndsData.resize(3 * nFacesTriangulationCount);
  baryCoordData.clear();
  baryCoordData.resize(3 * nFacesTriangulationCount);
  edgeIsRealData.clear();
  edgeIsRealData.resize(3 * nFacesTriangulationCount);

  // validate the face-vertex indices
  for (size_t iV : faceIndsEntries) {
    if (iV >= vertexPositions.size())
      throw std::logic_error("[polyscope] SurfaceMesh " + name + " has face vertex index " + std::to_string(iV) +
                             " out of bounds for number of vertices " + std::to_string(vertexPositions.size()));
  }

  // construct the triangualted draw list and all other related data
  size_t iTriFace = 0;
  for (size_t iF = 0; iF < numFaces; iF++) {
    size_t D = faceIndsStart[iF + 1] - faceIndsStart[iF];

    size_t iStart = faceIndsStart[iF];
    uint32_t vRoot = faceIndsEntries[iStart];

    // implicitly triangulate from root
    for (size_t j = 1; (j + 1) < D; j++) {
      uint32_t vB = faceIndsEntries[iStart + j];
      uint32_t vC = faceIndsEntries[iStart + ((j + 1) % D)];

      // triangle vertex indices
      triangleVertexIndsData[3 * iTriFace + 0] = vRoot;
      triangleVertexIndsData[3 * iTriFace + 1] = vB;
      triangleVertexIndsData[3 * iTriFace + 2] = vC;

      // triangle face indices
      for (size_t k = 0; k < 3; k++) triangleFaceIndsData[3 * iTriFace + k] = iF;

      // barycentric coordinates
      baryCoordData[3 * iTriFace + 0] = glm::vec3{1., 0., 0.};
      baryCoordData[3 * iTriFace + 1] = glm::vec3{0., 1., 0.};
      baryCoordData[3 * iTriFace + 2] = glm::vec3{0., 0., 1.};

      // internal edges for triangulated polygons
      glm::vec3 edgeRealV{0., 1., 0.};
      if (j == 1) {
        edgeRealV.x = 1.;
      }
      if (j + 2 == D) {
        edgeRealV.z = 1.;
      }
      for (size_t k = 0; k < 3; k++) edgeIsRealData[3 * iTriFace + k] = edgeRealV;

      iTriFace++;
    }
  }

  vertexDataSize = nVertices();
  faceDataSize = nFaces();
  // edgeDataSize = ... we don't know this yet, gets set below
  halfedgeDataSize = nHalfedges();
  cornerDataSize = nCorners();
}

// =================================================
// =====    Lazily-Populated Connectivity   ========
// =================================================

void SurfaceMesh::computeTriangleEdgeInds() {

  if (edgePerm.empty())
    throw std::logic_error(
        "[polyscope] SurfaceMesh " + name +
        " performed an operation which requires edge indices to be specified, but none have been set. "
        "Call setEdgePermutation().");

  triangleFaceInds.ensureHostBufferPopulated();
  triangleEdgeInds.data.resize(3 * 3 * nFaces());

  // used to loop over edges
  std::unordered_map<std::pair<size_t, size_t>, size_t, polyscope::hash_combine::hash<std::pair<size_t, size_t>>>
      seenEdgeInds;

  auto createEdgeKey = [&](size_t a, size_t b) -> std::pair<size_t, size_t> {
    return std::make_pair(std::min(a, b), std::max(a, b));
  };

  size_t psEdgeInd = 0; // polyscope's edge index, iterated according to Polyscope's canonical ordering
  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t start = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - start;

    if (D != 3) {
      throw std::logic_error(
          "[polyscope] SurfaceMesh " + name +
          " attempted to access triangle-edge indices, but it has non-triangular faces. These indices are "
          "only well-defined on a pure-triangular mesh.");
    }


    glm::uvec3 thisTriInds{0, 0, 0};
    for (size_t j = 0; j < 3; j++) {
      size_t vA = triangleFaceInds.data[3 * iF + j];
      size_t vB = triangleFaceInds.data[3 * iF + ((j + 1) % 3)];

      std::pair<size_t, size_t> key = createEdgeKey(vA, vB);

      size_t thisEdgeInd;
      if (seenEdgeInds.find(key) == seenEdgeInds.end()) {
        // process a new edge in the canonical ordering
        if (psEdgeInd >= edgePerm.size()) {
          throw std::logic_error("[polyscope] SurfaceMesh " + name +
                                 " edge indexing out of bounds. Did you pass an edge ordering that is too short?");
        }
        thisEdgeInd = edgePerm[psEdgeInd];
        seenEdgeInds[key] = thisEdgeInd;
        psEdgeInd++;
      } else {
        // we've processed this edge before, retrieve its assigned index
        thisEdgeInd = seenEdgeInds[key];
      }

      thisTriInds[j] = thisEdgeInd;
    }

    for (size_t j = 0; j < 3; j++) {
      for (size_t k = 0; k < 3; k++) {
        triangleEdgeInds.data[9 * iF + 3 * j + k] = thisTriInds[k];
      }
    }
  }

  nEdgesCount = psEdgeInd;
  triangleEdgeInds.markHostBufferUpdated();
}

void SurfaceMesh::computeTriangleHalfedgeInds() {

  triangleHalfedgeInds.data.clear();
  triangleHalfedgeInds.data.reserve(3 * nFacesTriangulation());

  bool haveCustomIndex = !halfedgePerm.empty();

  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t iStart = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - iStart;

    // emit the data for triangles triangulating this face
    for (size_t j = 1; (j + 1) < D; j++) {

      // FORNOW: for polygonal faces, substitute the opposite-edge value for all internal edges of the triangulation

      uint32_t he0 = iStart + j; // this is a dummy value due to triangulation of polygons
      uint32_t he1 = iStart + j; // this is the actual right value for the opposite edge
      uint32_t he2 = iStart + j; // this is a dummy value due to triangulation of polygons

      // substitute non-dummy values for first and last edge if this is not an internal tri
      if (j == 1) he0 = iStart;
      if (j + 2 == D) he2 = iStart + D - 1;

      if (haveCustomIndex) {
        he0 = halfedgePerm[he0];
        he1 = halfedgePerm[he1];
        he2 = halfedgePerm[he2];
      }

      for (size_t k = 0; k < 3; k++) {
        triangleHalfedgeInds.data.push_back(he0);
        triangleHalfedgeInds.data.push_back(he1);
        triangleHalfedgeInds.data.push_back(he2);
      }
    }
  }

  triangleHalfedgeInds.markHostBufferUpdated();
}

void SurfaceMesh::computeTriangleCornerInds() {

  triangleCornerInds.data.clear();
  triangleCornerInds.data.reserve(3 * nFacesTriangulation());

  bool haveCustomIndex = !cornerPerm.empty();

  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t iStart = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - iStart;

    // emit the data for triangles triangulating this face
    for (size_t j = 1; (j + 1) < D; j++) {
      uint32_t c0 = iStart;
      uint32_t c1 = iStart + j;
      uint32_t c2 = iStart + j + 1;

      if (haveCustomIndex) {
        c0 = cornerPerm[c0];
        c1 = cornerPerm[c1];
        c2 = cornerPerm[c2];
      }


      for (size_t k = 0; k < 3; k++) {
        triangleCornerInds.data.push_back(c0);
        triangleCornerInds.data.push_back(c1);
        triangleCornerInds.data.push_back(c2);
      }
    }
  }

  triangleCornerInds.markHostBufferUpdated();
}


// =================================================
// ========    Geometric Quantities      ==========
// =================================================


size_t SurfaceMesh::nVertices() { return vertexPositions.size(); }

void SurfaceMesh::computeFaceNormals() {

  vertexPositions.ensureHostBufferPopulated();

  faceNormals.data.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t iStart = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - iStart;

    glm::vec3 fN{0., 0., 0.};
    if (D == 3) {
      glm::vec3 pA = vertexPositions.data[faceIndsEntries[iStart + 0]];
      glm::vec3 pB = vertexPositions.data[faceIndsEntries[iStart + 1]];
      glm::vec3 pC = vertexPositions.data[faceIndsEntries[iStart + 2]];
      fN = glm::cross(pB - pA, pC - pA);
    } else {
      for (size_t j = 0; j < D; j++) {
        glm::vec3 pA = vertexPositions.data[faceIndsEntries[iStart + j]];
        glm::vec3 pB = vertexPositions.data[faceIndsEntries[iStart + (j + 1) % D]];
        glm::vec3 pC = vertexPositions.data[faceIndsEntries[iStart + (j + 2) % D]];
        fN += glm::cross(pC - pB, pA - pB);
      }
    }
    fN = glm::normalize(fN);
    faceNormals.data[iF] = fN;
  }

  faceNormals.markHostBufferUpdated();
}

void SurfaceMesh::computeFaceCenters() {

  vertexPositions.ensureHostBufferPopulated();

  faceCenters.data.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t start = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - start;
    glm::vec3 faceCenter{0., 0., 0.};
    for (size_t j = 0; j < D; j++) {
      glm::vec3 pA = vertexPositions.data[faceIndsEntries[start + j]];
      faceCenter += pA;
    }
    faceCenter /= D;
    faceCenters.data[iF] = faceCenter;
  }

  faceCenters.markHostBufferUpdated();
}

void SurfaceMesh::computeFaceAreas() {

  vertexPositions.ensureHostBufferPopulated();

  faceAreas.data.resize(nFaces());

  // Loop over faces to compute face-valued quantities
  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t start = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - start;

    // Compute a face normal
    double fA;
    if (D == 3) {
      glm::vec3 pA = vertexPositions.data[faceIndsEntries[start + 0]];
      glm::vec3 pB = vertexPositions.data[faceIndsEntries[start + 1]];
      glm::vec3 pC = vertexPositions.data[faceIndsEntries[start + 2]];
      glm::vec3 fN = glm::cross(pB - pA, pC - pA);
      fA = 0.5 * glm::length(fN);
    } else {
      fA = 0;
      glm::vec3 pRoot = vertexPositions.data[faceIndsEntries[start]];
      for (size_t j = 1; j + 1 < D; j++) {
        glm::vec3 pA = vertexPositions.data[faceIndsEntries[start + j]];
        glm::vec3 pB = vertexPositions.data[faceIndsEntries[start + j + 1]];
        fA += 0.5 * glm::length(glm::cross(pA - pRoot, pB - pRoot));
      }
    }

    faceAreas.data[iF] = fA;
  }

  faceAreas.markHostBufferUpdated();
}


void SurfaceMesh::computeVertexNormals() {

  faceNormals.ensureHostBufferPopulated();
  faceAreas.ensureHostBufferPopulated();

  vertexNormals.data.resize(nVertices());

  const glm::vec3 zero{0., 0., 0.};

  std::fill(vertexNormals.data.begin(), vertexNormals.data.end(), zero);

  // Accumulate quantities from each face
  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t start = faceIndsStart[iF];
    size_t D = faceIndsStart[iF + 1] - start;
    for (size_t j = 0; j < D; j++) {
      size_t iV = faceIndsEntries[start + j];
      vertexNormals.data[iV] += faceNormals.data[iF] * static_cast<float>(faceAreas.data[iF]);
    }
  }

  // Normalize
  for (size_t iV = 0; iV < nVertices(); iV++) {
    vertexNormals.data[iV] = glm::normalize(vertexNormals.data[iV]);
  }

  vertexNormals.markHostBufferUpdated();
}

void SurfaceMesh::computeVertexAreas() {

  faceAreas.ensureHostBufferPopulated();

  vertexAreas.data.resize(nVertices());
  std::fill(vertexAreas.data.begin(), vertexAreas.data.end(), 0.);

  // Accumulate quantities from each face
  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t D = faceIndsStart[iF + 1] - faceIndsStart[iF];
    size_t start = faceIndsStart[iF];
    for (size_t j = 0; j < D; j++) {
      size_t iV = faceIndsEntries[start + j];
      vertexAreas.data[iV] += faceAreas.data[iF] / D;
    }
  }

  vertexAreas.markHostBufferUpdated();
}

void SurfaceMesh::computeDefaultFaceTangentSpaces() {

  vertexPositions.ensureHostBufferPopulated();
  faceNormals.ensureHostBufferPopulated();

  defaultFaceTangentSpaces.data.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t D = faceIndsStart[iF + 1] - faceIndsStart[iF];
    if (D != 3)
      throw std::logic_error("[polyscope] Default face tangent spaces only available for pure-triangular meshes");

    size_t start = faceIndsStart[iF];

    glm::vec3 pA = vertexPositions.data[faceIndsEntries[start + 0]];
    glm::vec3 pB = vertexPositions.data[faceIndsEntries[start + 1]];
    glm::vec3 N = faceNormals.data[iF];

    glm::vec3 basisX = pB - pA;
    basisX = glm::normalize(basisX - N * glm::dot(N, basisX));

    glm::vec3 basisY = glm::normalize(-glm::cross(basisX, N));

    defaultFaceTangentSpaces.data[iF][0] = basisX;
    defaultFaceTangentSpaces.data[iF][1] = basisY;
  }

  defaultFaceTangentSpaces.markHostBufferUpdated();
}

// === Edge Lengths ===

// void SurfaceMesh::computeEdgeLengths() {
//
//   vertexPositions.ensureHostBufferPopulated();
//
//   edgeLengths.data.resize(nEdges());
//
//   // Compute edge lengths
//   for (size_t iF = 0; iF < nFaces(); iF++) {
//     size_t D = faceIndsStart[iF + 1] - faceIndsStart[iF];
//     size_t start = faceIndsStart[iF];
//     for (size_t j = 0; j < D; j++) {
//       size_t iA = faceIndsEntries[start + j];
//       size_t iB = faceIndsEntries[start + (j + 1) % D];
//       glm::vec3 pA = vertexPositions.data[iA];
//       glm::vec3 pB = vertexPositions.data[iB];
//       edgeLengths.data[edgeIndices[iF][j]] = glm::length(pA - pB);
//     }
//   }
//
//   edgeLengths.markHostBufferUpdated();
// }

void SurfaceMesh::checkHaveVertexTangentSpaces() {
  if (vertexTangentSpaces.hasData()) return;
  throw std::logic_error("[polyscope] Operation requires vertex tangent spaces for SurfaceMesh " + name +
                         ", but no tangent spaces have been set. Set them with setVertexTangentBasisX() to continue.");
}
void SurfaceMesh::checkHaveFaceTangentSpaces() {
  if (faceTangentSpaces.hasData()) return;
  throw std::logic_error("[polyscope] Operation requires face tangent spaces for SurfaceMesh " + name +
                         ", but no tangent spaces have been set. Set them with setFaceTangentBasisX() to continue.");
}

void SurfaceMesh::checkTriangular() {
  if (nFacesTriangulation() != nFaces()) {
    throw std::logic_error("[polyscope] Cannot proceed, SurfaceMesh " + name + " is not a triangular mesh.");
  }
}

void SurfaceMesh::ensureHaveManifoldConnectivity() {
  if (!twinHalfedge.empty()) return; // already populated

  triangleVertexInds.ensureHostBufferPopulated();

  twinHalfedge.resize(nHalfedges());

  // Maps from edge (sorted) to all halfedges incident on that edge
  std::unordered_map<std::pair<size_t, size_t>, std::vector<size_t>,
                     polyscope::hash_combine::hash<std::pair<size_t, size_t>>>
      edgeInds;

  // Fill out faceForHalfedge and populate edge lookup map
  for (size_t iF = 0; iF < nFacesTriangulation(); iF++) {
    for (size_t j = 0; j < 3; j++) {
      size_t iV = triangleVertexInds.data[3 * iF + j];
      size_t iVNext = triangleVertexInds.data[3 * iF + ((j + 1) % 3)];
      size_t iHe = 3 * iF + j;


      std::pair<size_t, size_t> edgeKey(std::min(iV, iVNext), std::max(iV, iVNext));

      // Make sure the key is populated
      auto it = edgeInds.find(edgeKey);
      if (it == edgeInds.end()) {
        it = edgeInds.insert(it, {edgeKey, std::vector<size_t>()});
      }

      // Add this halfedge to the entry
      it->second.push_back(iHe);
    }
  }

  // Second walk through, setting twins
  for (size_t iF = 0; iF < nFacesTriangulation(); iF++) {
    for (size_t j = 0; j < 3; j++) {
      size_t iV = triangleVertexInds.data[3 * iF + j];
      size_t iVNext = triangleVertexInds.data[3 * iF + ((j + 1) % 3)];
      size_t iHe = 3 * iF + j;

      std::pair<size_t, size_t> edgeKey(std::min(iV, iVNext), std::max(iV, iVNext));
      std::vector<size_t>& edgeHalfedges = edgeInds.find(edgeKey)->second;

      // Pick the first halfedge we find which is not this one
      size_t myTwin = INVALID_IND;
      for (size_t t : edgeHalfedges) {
        if (t != iHe) {
          myTwin = t;
          break;
        }
      }

      twinHalfedge[iHe] = myTwin;
    }
  }
}

void SurfaceMesh::draw() {
  if (!isEnabled()) {
    return;
  }

  render::engine->setBackfaceCull(backFacePolicy.get() == BackFacePolicy::Cull);

  // If no quantity is drawing the surface, we should draw it
  if (dominantQuantity == nullptr) {

    if (program == nullptr) {
      prepare();

      // do this now to reduce lag when picking later, etc
      // FIXME
      // preparePick();
    }

    // Set uniforms
    setStructureUniforms(*program);
    setSurfaceMeshUniforms(*program);
    program->setUniform("u_baseColor", getSurfaceColor());

    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }

  render::engine->setBackfaceCull(); // return to default setting

  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void SurfaceMesh::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  render::engine->setBackfaceCull(backFacePolicy.get() == BackFacePolicy::Cull);

  for (auto& x : quantities) {
    x.second->drawDelayed();
  }

  render::engine->setBackfaceCull(); // return to default setting

  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}

void SurfaceMesh::drawPick() {
  if (!isEnabled()) {
    return;
  }

  if (pickProgram == nullptr) {
    preparePick();
  }

  render::engine->setBackfaceCull(backFacePolicy.get() == BackFacePolicy::Cull);

  // Set uniforms
  setStructureUniforms(*pickProgram);

  pickProgram->draw();

  render::engine->setBackfaceCull(); // return to default setting
}

void SurfaceMesh::prepare() {
  // clang-format off
  program = render::engine->requestShader(
      "MESH", 
      addSurfaceMeshRules({"SHADE_BASECOLOR"})
  );
  // clang-format on

  // Populate draw buffers
  setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, getMaterial());
}

void SurfaceMesh::preparePick() {

  // clang-format off
  pickProgram = render::engine->requestShader(
      "MESH", 
      addSurfaceMeshRules({"MESH_PROPAGATE_PICK"}, true, false),
      render::ShaderReplacementDefaults::Pick
  );
  // clang-format on

  // Populate draw buffers
  setMeshGeometryAttributes(*pickProgram);
  setMeshPickAttributes(*pickProgram);
}

void SurfaceMesh::setMeshGeometryAttributes(render::ShaderProgram& p) {
  if (p.hasAttribute("a_vertexPositions")) {
    p.setAttribute("a_vertexPositions", vertexPositions.getIndexedRenderAttributeBuffer(&triangleVertexInds));
  }
  if (p.hasAttribute("a_vertexNormals")) {

    if (getShadeStyle() == MeshShadeStyle::Smooth) {
      p.setAttribute("a_vertexNormals", vertexNormals.getIndexedRenderAttributeBuffer(&triangleVertexInds));
    } else {
      // these aren't actually used in in the automatically-generated case, but the shader is set up in a lazy way so it
      // is still needed
      p.setAttribute("a_vertexNormals", faceNormals.getIndexedRenderAttributeBuffer(&triangleFaceInds));
    }
  }
  if (p.hasAttribute("a_normal")) {
    p.setAttribute("a_normal", faceNormals.getIndexedRenderAttributeBuffer(&triangleFaceInds));
  }
  if (p.hasAttribute("a_barycoord")) {
    p.setAttribute("a_barycoord", baryCoord.getRenderAttributeBuffer());
  }
  if (p.hasAttribute("a_edgeIsReal")) {
    p.setAttribute("a_edgeIsReal", edgeIsReal.getRenderAttributeBuffer());
  }
  if (wantsCullPosition()) {
    p.setAttribute("a_cullPos", faceCenters.getIndexedRenderAttributeBuffer(&triangleFaceInds));
  }
}

void SurfaceMesh::setMeshPickAttributes(render::ShaderProgram& p) {

  // TODO in principle all of the data this shader needs is already available on the GPU via the [...]Inds attribute
  // buffers. We could move the encoding / offsetting logic to happen in a shader with some uniforms, and avoid any
  // CPU-side processing. Maybe the solution is to directly render ints?

  // make sure we have the relevant indexing data
  triangleVertexInds.ensureHostBufferPopulated();
  triangleFaceInds.ensureHostBufferPopulated();
  if (edgesHaveBeenUsed) triangleEdgeInds.ensureHostBufferPopulated();
  if (halfedgesHaveBeenUsed) triangleHalfedgeInds.ensureHostBufferPopulated();

  // Get element indices
  size_t totalPickElements = nVertices() + nFaces();
  if (edgesHaveBeenUsed) totalPickElements += nEdges();
  if (halfedgesHaveBeenUsed) totalPickElements += nHalfedges();

  // In "local" indices, indexing elements only within this mesh, used for reading later
  facePickIndStart = nVertices();
  edgePickIndStart = facePickIndStart + nFaces();
  halfedgePickIndStart = edgePickIndStart;
  if (edgesHaveBeenUsed) {
    halfedgePickIndStart += nEdges();
  }

  // In "global" indices, indexing all elements in the scene, used to fill buffers for drawing here
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);
  size_t vertexGlobalPickIndStart = pickStart;
  size_t faceGlobalPickIndStart = pickStart + facePickIndStart;
  size_t edgeGlobalPickIndStart = pickStart + edgePickIndStart;
  size_t halfedgeGlobalPickIndStart = pickStart + halfedgePickIndStart;

  // == Fill buffers
  std::vector<std::array<glm::vec3, 3>> vertexColors, edgeColors, halfedgeColors;
  std::vector<glm::vec3> faceColor;

  // Reserve space
  vertexColors.reserve(3 * nFacesTriangulation());
  edgeColors.reserve(3 * nFacesTriangulation());
  halfedgeColors.reserve(3 * nFacesTriangulation());
  faceColor.reserve(3 * nFacesTriangulation());


  // Build all quantities in each face
  size_t iFTri = 0;
  for (size_t iF = 0; iF < nFaces(); iF++) {
    size_t D = faceIndsStart[iF + 1] - faceIndsStart[iF];

    glm::vec3 fColor = pick::indToVec(iF + faceGlobalPickIndStart);

    for (size_t j = 1; (j + 1) < D; j++) {

      // == Build face & vertex index data

      // clang-format off
      std::array<glm::vec3, 3> vColor = {
        pick::indToVec(triangleVertexInds.data[3*iFTri + 0] + vertexGlobalPickIndStart),
        pick::indToVec(triangleVertexInds.data[3*iFTri + 1] + vertexGlobalPickIndStart),
        pick::indToVec(triangleVertexInds.data[3*iFTri + 2] + vertexGlobalPickIndStart),
      };
      // clang-format on

      for (int j = 0; j < 3; j++) {
        faceColor.push_back(fColor);
        vertexColors.push_back(vColor);
      }


      // == Build edge index data, if needed

      if (edgesHaveBeenUsed) {
        // clang-format off
        std::array<glm::vec3, 3> eColor = { 
          fColor, 
          pick::indToVec(triangleEdgeInds.data[9*iFTri + 1] + edgeGlobalPickIndStart), 
          fColor
        };
        // clang-format on
        if (j == 1) eColor[0] = pick::indToVec(triangleEdgeInds.data[9 * iFTri + 0] + edgeGlobalPickIndStart);
        if (j + 2 == D) eColor[2] = pick::indToVec(triangleEdgeInds.data[9 * iFTri + 2] + edgeGlobalPickIndStart);

        for (int j = 0; j < 3; j++) edgeColors.push_back(eColor);
      } else {
        for (int j = 0; j < 3; j++) edgeColors.push_back({fColor, fColor, fColor});
      }

      // == Build halfedge index data, if needed

      if (halfedgesHaveBeenUsed) {
        // clang-format off
        std::array<glm::vec3, 3> heColor = { 
          fColor, 
          pick::indToVec(triangleHalfedgeInds.data[9*iFTri + 1] + halfedgeGlobalPickIndStart), 
          fColor
        };
        // clang-format on
        if (j == 1) heColor[0] = pick::indToVec(triangleHalfedgeInds.data[9 * iFTri + 0] + halfedgeGlobalPickIndStart);
        if (j + 2 == D)
          heColor[2] = pick::indToVec(triangleHalfedgeInds.data[9 * iFTri + 2] + halfedgeGlobalPickIndStart);

        for (int j = 0; j < 3; j++) halfedgeColors.push_back(heColor);
      } else {
        for (int j = 0; j < 3; j++) halfedgeColors.push_back({fColor, fColor, fColor});
      }

      // TODO corners?

      iFTri++;
    }
  }

  // Store data in buffers
  pickProgram->setAttribute<glm::vec3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_edgeColors", edgeColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_halfedgeColors", halfedgeColors);
  pickProgram->setAttribute("a_faceColor", faceColor);
}


std::vector<std::string> SurfaceMesh::addSurfaceMeshRules(std::vector<std::string> initRules, bool withMesh,
                                                          bool withSurfaceShade) {
  initRules = addStructureRules(initRules);

  if (withMesh) {

    if (withSurfaceShade) {
      // rules that only get used when we're shading the surface of the mesh
      if (getEdgeWidth() > 0) {
        initRules.push_back("MESH_WIREFRAME");
      }

      if (shadeStyle.get() == MeshShadeStyle::TriFlat) {
        initRules.push_back("MESH_COMPUTE_NORMAL_FROM_POSITION");
      }

      if (backFacePolicy.get() == BackFacePolicy::Different) {
        initRules.push_back("MESH_BACKFACE_DARKEN");
      }
      if (backFacePolicy.get() == BackFacePolicy::Custom) {
        initRules.push_back("MESH_BACKFACE_DIFFERENT");
      }
    }

    if (backFacePolicy.get() == BackFacePolicy::Identical) {
      initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
    }

    if (backFacePolicy.get() == BackFacePolicy::Different) {
      initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
    }

    if (backFacePolicy.get() == BackFacePolicy::Custom) {
      initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
    }

    if (wantsCullPosition()) {
      initRules.push_back("MESH_PROPAGATE_CULLPOS");
    }
  }
  return initRules;
}

void SurfaceMesh::setSurfaceMeshUniforms(render::ShaderProgram& p) {
  if (getEdgeWidth() > 0) {
    p.setUniform("u_edgeWidth", getEdgeWidth() * render::engine->getCurrentPixelScaling());
    p.setUniform("u_edgeColor", getEdgeColor());
  }
  if (backFacePolicy.get() == BackFacePolicy::Custom) {
    p.setUniform("u_backfaceColor", getBackFaceColor());
  }
  if (shadeStyle.get() == MeshShadeStyle::TriFlat) {
    glm::mat4 P = view::getCameraPerspectiveMatrix();
    glm::mat4 Pinv = glm::inverse(P);
    p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
    p.setUniform("u_viewport", render::engine->getCurrentViewport());
  }
}


void SurfaceMesh::buildPickUI(size_t localPickID) {

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

glm::vec2 SurfaceMesh::projectToScreenSpace(glm::vec3 coord) {

  glm::mat4 viewMat = getModelView();
  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  glm::vec4 coord4(coord.x, coord.y, coord.z, 1.0);
  glm::vec4 screenPoint = projMat * viewMat * coord4;

  return glm::vec2{screenPoint.x, screenPoint.y} / screenPoint.w;
}

void SurfaceMesh::buildVertexInfoGui(size_t vInd) {

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

void SurfaceMesh::buildFaceInfoGui(size_t fInd) {
  size_t displayInd = fInd;
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

void SurfaceMesh::buildEdgeInfoGui(size_t eInd) {
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

void SurfaceMesh::buildHalfedgeInfoGui(size_t heInd) {
  size_t displayInd = heInd;
  if (halfedgePerm.size() > 0) {
    displayInd = halfedgePerm[heInd];
  }
  ImGui::TextUnformatted(("Halfedge #" + std::to_string(displayInd)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildHalfedgeInfoGUI(heInd);
  }

  ImGui::Indent(-20.);
}


void SurfaceMesh::buildCustomUI() {

  // Print stats
  long long int nVertsL = static_cast<long long int>(nVertices());
  long long int nFacesL = static_cast<long long int>(nFaces());
  ImGui::Text("#verts: %lld  #faces: %lld", nVertsL, nFacesL);

  { // Colors
    if (ImGui::ColorEdit3("Color", &surfaceColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setSurfaceColor(surfaceColor.get());
    ImGui::SameLine();
  }


  { // Flat shading or smooth shading?
    ImGui::SameLine();
    ImGui::PushItemWidth(85);

    auto styleName = [](const MeshShadeStyle& m) -> std::string {
      switch (m) {
      case MeshShadeStyle::Smooth:
        return "Smooth";
      case MeshShadeStyle::Flat:
        return "Flat";
      case MeshShadeStyle::TriFlat:
        return "Tri Flat";
      }
      return "";
    };

    if (ImGui::BeginCombo("##Mode", styleName(getShadeStyle()).c_str())) {
      for (MeshShadeStyle s : {MeshShadeStyle::Flat, MeshShadeStyle::Smooth, MeshShadeStyle::TriFlat}) {
        std::string sName = styleName(s);
        if (ImGui::Selectable(sName.c_str(), getShadeStyle() == s)) {
          setShadeStyle(s);
        }
      }
      ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
  }

  { // Edge options
    ImGui::SameLine();
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
      ImGui::PushItemWidth(75);
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


  { // Backface color (only visible if policy is selected)
    if (backFacePolicy.get() == BackFacePolicy::Custom) {
      if (ImGui::ColorEdit3("Backface Color", &backFaceColor.get()[0], ImGuiColorEditFlags_NoInputs))
        setBackFaceColor(backFaceColor.get());
    }
  }
}


void SurfaceMesh::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }

  // backfaces
  if (ImGui::BeginMenu("Back Face Policy")) {
    if (ImGui::MenuItem("identical shading", NULL, backFacePolicy.get() == BackFacePolicy::Identical))
      setBackFacePolicy(BackFacePolicy::Identical);
    if (ImGui::MenuItem("different shading", NULL, backFacePolicy.get() == BackFacePolicy::Different))
      setBackFacePolicy(BackFacePolicy::Different);
    if (ImGui::MenuItem("custom shading", NULL, backFacePolicy.get() == BackFacePolicy::Custom))
      setBackFacePolicy(BackFacePolicy::Custom);
    if (ImGui::MenuItem("cull", NULL, backFacePolicy.get() == BackFacePolicy::Cull))
      setBackFacePolicy(BackFacePolicy::Cull);
    ImGui::EndMenu();
  }
}

void SurfaceMesh::recomputeGeometryIfPopulated() {
  faceNormals.recomputeIfPopulated();
  faceCenters.recomputeIfPopulated();
  faceAreas.recomputeIfPopulated();
  vertexNormals.recomputeIfPopulated();
  vertexAreas.recomputeIfPopulated();
  // edgeLengths.recomputeIfPopulated();
}

void SurfaceMesh::refresh() {
  recomputeGeometryIfPopulated();

  program.reset();
  pickProgram.reset();
  requestRedraw();
  QuantityStructure<SurfaceMesh>::refresh(); // call base class version, which refreshes quantities
}

void SurfaceMesh::updateObjectSpaceBounds() {

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

std::string SurfaceMesh::typeName() { return structureTypeName; }

long long int SurfaceMesh::selectVertex() {

  // Make sure we can see edges
  float oldEdgeWidth = getEdgeWidth();
  setEdgeWidth(1.);
  this->setEnabled(true);

  long long int returnVertInd = -1;

  // Register the callback which creates the UI and does the hard work
  auto focusedPopupUI = [&]() {
    { // Create a window with instruction and a close button.
      static bool showWindow = true;
      ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Once);
      ImGui::Begin("Select vertex", &showWindow);

      ImGui::PushItemWidth(300);
      ImGui::TextUnformatted("Hold ctrl and left-click to select a vertex");
      ImGui::Separator();

      // Choose by number
      ImGui::PushItemWidth(300);
      static int iV = -1;
      ImGui::InputInt("index", &iV);
      if (ImGui::Button("Select by index")) {
        if (iV >= 0 && (size_t)iV < nVertices()) {
          returnVertInd = iV;
          popContext();
        }
      }
      ImGui::PopItemWidth();

      ImGui::Separator();
      if (ImGui::Button("Abort")) {
        popContext();
      }

      ImGui::End();
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && !io.WantCaptureMouse && ImGui::IsMouseClicked(0)) {

      ImGuiIO& io = ImGui::GetIO();

      // API is a giant mess..
      size_t pickInd;
      ImVec2 p = ImGui::GetMousePos();
      std::pair<Structure*, size_t> pickVal =
          pick::evaluatePickQuery(io.DisplayFramebufferScale.x * p.x, io.DisplayFramebufferScale.y * p.y);

      if (pickVal.first == this) {

        if (pickVal.second < nVertices()) {
          returnVertInd = pickVal.second;
          popContext();
        }
      }
    }
  };

  // Pass control to the context we just created
  pushContext(focusedPopupUI);

  setEdgeWidth(oldEdgeWidth); // restore edge setting

  return returnVertInd;
}


void SurfaceMesh::markEdgesAsUsed() {
  edgesHaveBeenUsed = true;
  // immediately compute edge-related connectivity info, and also repopulate the pick buffer so edges can be picked
  computeTriangleEdgeInds();
  pickProgram.reset();
}

void SurfaceMesh::markHalfedgesAsUsed() {
  halfedgesHaveBeenUsed = true;
  // repopulate the pick buffer so halfedges can be picked
  pickProgram.reset();
}

// === Option getters and setters

// DEPRECATED!
SurfaceMesh* SurfaceMesh::setSmoothShade(bool isSmooth) {
  if (isSmooth) {
    return setShadeStyle(MeshShadeStyle::Smooth);
  } else {
    return setShadeStyle(MeshShadeStyle::Flat);
  }
}
// DEPRECATED!
bool SurfaceMesh::isSmoothShade() { return getShadeStyle() == MeshShadeStyle::Smooth; }

SurfaceMesh* SurfaceMesh::setBackFaceColor(glm::vec3 val) {
  backFaceColor.set(val);
  requestRedraw();
  return this;
}

glm::vec3 SurfaceMesh::getBackFaceColor() { return backFaceColor.get(); }

SurfaceMesh* SurfaceMesh::setSurfaceColor(glm::vec3 val) {
  surfaceColor = val;
  requestRedraw();
  return this;
}
glm::vec3 SurfaceMesh::getSurfaceColor() { return surfaceColor.get(); }

SurfaceMesh* SurfaceMesh::setEdgeColor(glm::vec3 val) {
  edgeColor = val;
  requestRedraw();
  return this;
}
glm::vec3 SurfaceMesh::getEdgeColor() { return edgeColor.get(); }

SurfaceMesh* SurfaceMesh::setMaterial(std::string m) {
  material = m;
  refresh(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
  requestRedraw();
  return this;
}
std::string SurfaceMesh::getMaterial() { return material.get(); }

SurfaceMesh* SurfaceMesh::setEdgeWidth(double newVal) {
  edgeWidth = newVal;
  refresh();
  requestRedraw();
  return this;
}
double SurfaceMesh::getEdgeWidth() { return edgeWidth.get(); }

SurfaceMesh* SurfaceMesh::setBackFacePolicy(BackFacePolicy newPolicy) {
  backFacePolicy = newPolicy;
  refresh();
  requestRedraw();
  return this;
}
BackFacePolicy SurfaceMesh::getBackFacePolicy() { return backFacePolicy.get(); }

SurfaceMesh* SurfaceMesh::setShadeStyle(MeshShadeStyle newStyle) {
  shadeStyle = newStyle;
  refresh();
  requestRedraw();
  return this;
}
MeshShadeStyle SurfaceMesh::getShadeStyle() { return shadeStyle.get(); }

// === Quantity adders


SurfaceVertexColorQuantity* SurfaceMesh::addVertexColorQuantityImpl(std::string name,
                                                                    const std::vector<glm::vec3>& colors) {
  SurfaceVertexColorQuantity* q = new SurfaceVertexColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

SurfaceFaceColorQuantity* SurfaceMesh::addFaceColorQuantityImpl(std::string name,
                                                                const std::vector<glm::vec3>& colors) {
  SurfaceFaceColorQuantity* q = new SurfaceFaceColorQuantity(name, *this, colors);
  addQuantity(q);
  return q;
}

SurfaceVertexScalarQuantity* SurfaceMesh::addVertexDistanceQuantityImpl(std::string name,
                                                                        const std::vector<double>& data) {
  SurfaceVertexScalarQuantity* q = new SurfaceVertexScalarQuantity(name, data, *this, DataType::MAGNITUDE);

  q->setIsolinesEnabled(true);
  q->setIsolineWidth(0.02, true);

  addQuantity(q);
  return q;
}

SurfaceVertexScalarQuantity* SurfaceMesh::addVertexSignedDistanceQuantityImpl(std::string name,
                                                                              const std::vector<double>& data) {
  SurfaceVertexScalarQuantity* q = new SurfaceVertexScalarQuantity(name, data, *this, DataType::SYMMETRIC);

  q->setIsolinesEnabled(true);
  q->setIsolineWidth(0.02, true);

  addQuantity(q);
  return q;
}

SurfaceCornerParameterizationQuantity*
SurfaceMesh::addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                             ParamCoordsType type) {
  // SurfaceCornerParameterizationQuantity* q = new SurfaceCornerParameterizationQuantity(
  //     name, applyPermutation(coords, cornerPerm), type, ParamVizStyle::CHECKER, *this);
  // addQuantity(q);
  //
  // return q;
  return nullptr; // TODO restore
}

SurfaceVertexParameterizationQuantity*
SurfaceMesh::addVertexParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                   ParamCoordsType type) {
  // SurfaceVertexParameterizationQuantity* q =
  //     new SurfaceVertexParameterizationQuantity(name, coords, type, ParamVizStyle::CHECKER, *this);
  // addQuantity(q);
  //
  // return q;
  return nullptr; // TODO restore
}

SurfaceVertexParameterizationQuantity*
SurfaceMesh::addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                  ParamCoordsType type) {
  // SurfaceVertexParameterizationQuantity* q =
  //     new SurfaceVertexParameterizationQuantity(name, coords, type, ParamVizStyle::LOCAL_CHECK, *this);
  // addQuantity(q);
  //
  // return q;
  return nullptr; // TODO restore
}

/*

SurfaceVertexCountQuantity* SurfaceMesh::addVertexCountQuantityImpl(std::string name,
                                                                    const std::vector<std::pair<size_t, int>>& values) {

  SurfaceVertexCountQuantity* q = new SurfaceVertexCountQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

SurfaceVertexIsolatedScalarQuantity*
SurfaceMesh::addVertexIsolatedScalarQuantityImpl(std::string name,
                                                 const std::vector<std::pair<size_t, double>>& values) {
  SurfaceVertexIsolatedScalarQuantity* q = new SurfaceVertexIsolatedScalarQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

SurfaceFaceCountQuantity* SurfaceMesh::addFaceCountQuantityImpl(std::string name,
                                                                const std::vector<std::pair<size_t, int>>& values) {
  SurfaceFaceCountQuantity* q = new SurfaceFaceCountQuantity(name, values, *this);
  addQuantity(q);
  return q;
}

SurfaceGraphQuantity* SurfaceMesh::addSurfaceGraphQuantityImpl(std::string name, const std::vector<glm::vec3>& nodes,
                                                               const std::vector<std::array<size_t, 2>>& edges) {
  SurfaceGraphQuantity* q = new SurfaceGraphQuantity(name, nodes, edges, *this);
  addQuantity(q);
  return q;
}

*/

SurfaceVertexScalarQuantity* SurfaceMesh::addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                      DataType type) {
  SurfaceVertexScalarQuantity* q = new SurfaceVertexScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

SurfaceFaceScalarQuantity* SurfaceMesh::addFaceScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                  DataType type) {
  SurfaceFaceScalarQuantity* q = new SurfaceFaceScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}


SurfaceEdgeScalarQuantity* SurfaceMesh::addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                  DataType type) {
  SurfaceEdgeScalarQuantity* q = new SurfaceEdgeScalarQuantity(name, applyPermutation(data, edgePerm), *this, type);
  addQuantity(q);
  markEdgesAsUsed();
  return q;
}

SurfaceHalfedgeScalarQuantity*
SurfaceMesh::addHalfedgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  SurfaceHalfedgeScalarQuantity* q =
      new SurfaceHalfedgeScalarQuantity(name, applyPermutation(data, halfedgePerm), *this, type);
  addQuantity(q);
  markHalfedgesAsUsed();
  return q;
}


SurfaceVertexVectorQuantity* SurfaceMesh::addVertexVectorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& vectors,
                                                                      VectorType vectorType) {
  SurfaceVertexVectorQuantity* q = new SurfaceVertexVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

SurfaceFaceVectorQuantity*
SurfaceMesh::addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType) {

  SurfaceFaceVectorQuantity* q = new SurfaceFaceVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

SurfaceFaceIntrinsicVectorQuantity*
SurfaceMesh::addFaceIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors,
                                                VectorType vectorType) {

  SurfaceFaceIntrinsicVectorQuantity* q = new SurfaceFaceIntrinsicVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}


SurfaceVertexIntrinsicVectorQuantity*
SurfaceMesh::addVertexIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors,
                                                  VectorType vectorType) {
  SurfaceVertexIntrinsicVectorQuantity* q = new SurfaceVertexIntrinsicVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

// Orientations is `true` if the canonical orientation of the edge points from the lower-indexed vertex to the
// higher-indexed vertex, and `false` otherwise.
SurfaceOneFormIntrinsicVectorQuantity*
SurfaceMesh::addOneFormIntrinsicVectorQuantityImpl(std::string name, const std::vector<double>& data,
                                                   const std::vector<char>& orientations) {
  SurfaceOneFormIntrinsicVectorQuantity* q = new SurfaceOneFormIntrinsicVectorQuantity(
      name, applyPermutation(data, edgePerm), applyPermutation(orientations, edgePerm), *this);
  addQuantity(q);
  return q;
}


void SurfaceMesh::setVertexTangentBasisXImpl(const std::vector<glm::vec3>& inputBasisX) {

  vertexNormals.ensureHostBufferPopulated();

  vertexTangentSpaces.data.resize(nVertices());

  for (size_t iV = 0; iV < nVertices(); iV++) {

    glm::vec3 basisX = inputBasisX[iV];
    glm::vec3 normal = vertexNormals.data[iV];

    // Project in to tangent defined by our normals
    basisX = glm::normalize(basisX - normal * glm::dot(normal, basisX));

    // Let basis Y complete the frame
    glm::vec3 basisY = glm::cross(normal, basisX);

    vertexTangentSpaces.data[iV][0] = basisX;
    vertexTangentSpaces.data[iV][1] = basisY;
  }

  vertexTangentSpaces.markHostBufferUpdated();
}

void SurfaceMesh::setFaceTangentBasisXImpl(const std::vector<glm::vec3>& inputBasisX) {

  faceNormals.ensureHostBufferPopulated();

  faceTangentSpaces.data.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {

    glm::vec3 basisX = inputBasisX[iF];
    glm::vec3 normal = faceNormals.data[iF];

    // Project in to tangent defined by our normals
    basisX = glm::normalize(basisX - normal * glm::dot(normal, basisX));

    // Let basis Y complete the frame
    glm::vec3 basisY = glm::cross(normal, basisX);

    faceTangentSpaces.data[iF][0] = basisX;
    faceTangentSpaces.data[iF][1] = basisY;
  }

  faceTangentSpaces.markHostBufferUpdated();
}


SurfaceMeshQuantity::SurfaceMeshQuantity(std::string name, SurfaceMesh& parentStructure, bool dominates)
    : QuantityS<SurfaceMesh>(name, parentStructure, dominates) {}
void SurfaceMeshQuantity::buildVertexInfoGUI(size_t vInd) {}
void SurfaceMeshQuantity::buildFaceInfoGUI(size_t fInd) {}
void SurfaceMeshQuantity::buildEdgeInfoGUI(size_t eInd) {}
void SurfaceMeshQuantity::buildHalfedgeInfoGUI(size_t heInd) {}

} // namespace polyscope

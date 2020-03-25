// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_mesh.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"

#include <unordered_map>
#include <utility>

using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string SurfaceMesh::structureTypeName = "Surface Mesh";


SurfaceMesh::SurfaceMesh(std::string name, const std::vector<glm::vec3>& vertexPositions,
                         const std::vector<std::vector<size_t>>& faceIndices)
    : QuantityStructure<SurfaceMesh>(name, typeName()), vertices(vertexPositions), faces(faceIndices),
      shadeSmooth(uniquePrefix() + "shadeSmooth", false),
      surfaceColor(uniquePrefix() + "surfaceColor", getNextUniqueColor()),
      edgeColor(uniquePrefix() + "edgeColor", glm::vec3{0., 0., 0.}),
      material(uniquePrefix() + "material", "clay"), edgeWidth(uniquePrefix() + "edgeWidth", 0.) {

  computeCounts();
  computeGeometryData();
}


void SurfaceMesh::computeCounts() {

  nFacesTriangulationCount = 0;
  nCornersCount = 0;
  nEdgesCount = 0;
  edgeIndices.resize(nFaces());
  halfedgeIndices.resize(nFaces());
  std::unordered_map<std::pair<size_t, size_t>, size_t, polyscope::hash_combine::hash<std::pair<size_t, size_t>>>
      edgeInds;
  size_t iF = 0;
  for (auto& face : faces) {
    if (face.size() < 3) {
      warning(name + " has face with degree < 3!");
      face = {0, 0, 0}; // (just to do _something_ so we don't crash in subsequent code)
    }
    nFacesTriangulationCount += std::max(static_cast<int>(face.size()) - 2, 0);
    edgeIndices[iF].resize(face.size());
    halfedgeIndices[iF].resize(face.size());

    for (size_t i = 0; i < face.size(); i++) {
      size_t vA = face[i];
      size_t vB = face[(i + 1) % face.size()];

      if (vA >= vertices.size()) {
        warning(name + " has face with vertex index out of vertices range",
                "face " + std::to_string(iF) + " has vertex index " + std::to_string(vA));

        // zero out the face index
        // (just to do _something_ so we don't crash in subsequent code)
        for (size_t j = 0; j < face.size(); j++) {
          face[j] = 0;
        }
        vA = face[i];
        vB = face[(i + 1) % face.size()];
      }

      std::pair<size_t, size_t> edgeKey(std::min(vA, vB), std::max(vA, vB));
      auto it = edgeInds.find(edgeKey);
      size_t edgeInd = 0;
      if (it == edgeInds.end()) {
        edgeInd = nEdgesCount;
        edgeInds.insert(it, {edgeKey, edgeInd});
        nEdgesCount++;
      } else {
        edgeInd = it->second;
      }

      edgeIndices[iF][i] = edgeInd;
      halfedgeIndices[iF][i] = nCornersCount++;
    }

    iF++;
  }

  // Default data sizes
  vertexDataSize = nVertices();
  faceDataSize = nFaces();
  edgeDataSize = nEdges();
  halfedgeDataSize = nHalfedges();
  cornerDataSize = nCorners();
}

void SurfaceMesh::computeGeometryData() {
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
}

void SurfaceMesh::ensureHaveManifoldConnectivity() {
  if (!faceForHalfedge.empty()) return; // already populated

  faceForHalfedge.resize(nHalfedges());
  twinHalfedge.resize(nHalfedges());

  // Maps from edge (sorted) to all halfedges incident on that edge
  std::unordered_map<std::pair<size_t, size_t>, std::vector<size_t>,
                     polyscope::hash_combine::hash<std::pair<size_t, size_t>>>
      edgeInds;

  // Fill out faceForHalfedge and populate edge lookup map
  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();


    for (size_t j = 0; j < D; j++) {
      size_t iV = face[j];
      size_t iVNext = face[(j + 1) % D];
      size_t iHe = halfedgeIndices[iF][j];

      faceForHalfedge[iHe] = iF;

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
  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();

    for (size_t j = 0; j < D; j++) {
      size_t iV = face[j];
      size_t iVNext = face[(j + 1) % D];
      size_t iHe = halfedgeIndices[iF][j];

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

void SurfaceMesh::ensureHaveFaceTangentSpaces() {
  if (faceTangentSpaces.size() > 0) return;

  faceTangentSpaces.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();
    if (D < 2) continue;

    glm::vec3 pA = vertices[face[0]];
    glm::vec3 pB = vertices[face[1]];
    glm::vec3 N = faceNormals[iF];

    glm::vec3 basisX = pB - pA;
    basisX = glm::normalize(basisX - N * glm::dot(N, basisX));

    glm::vec3 basisY = glm::normalize(-glm::cross(basisX, N));

    faceTangentSpaces[iF][0] = basisX;
    faceTangentSpaces[iF][1] = basisY;
  }
}

void SurfaceMesh::ensureHaveVertexTangentSpaces() {
  if (vertexTangentSpaces.size() > 0) return;

  vertexTangentSpaces.resize(nVertices());
  std::vector<char> hasTangent(nVertices(), false);

  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();
    if (D < 2) continue;

    for (size_t j = 0; j < D; j++) {

      size_t vA = face[j];
      size_t vB = face[(j + 1) % D];

      if (hasTangent[vA]) continue;

      glm::vec3 pA = vertices[vA];
      glm::vec3 pB = vertices[vB];
      glm::vec3 N = vertexNormals[vA];

      glm::vec3 basisX = pB - pA;
      basisX = basisX - N * glm::dot(N, basisX);

      glm::vec3 basisY = -glm::cross(basisX, N);

      vertexTangentSpaces[vA][0] = basisX;
      vertexTangentSpaces[vA][1] = basisY;

      hasTangent[vA] = true;
    }
  }
}

void SurfaceMesh::draw() {
  if (!isEnabled()) {
    return;
  }


  // If no quantity is drawing the surface, we should draw it
  if (dominantQuantity == nullptr) {

    if (program == nullptr) {
      prepare();

      // do these now to reduce lag when picking later, etc
      // prepareWireframe();
      preparePick();
    }

    // Set uniforms
    setTransformUniforms(*program);
    program->setUniform("u_basecolor", getSurfaceColor());

    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }

  // Draw the wireframe
  if (getEdgeWidth() > 0) {
    if (wireframeProgram == nullptr) {
      prepareWireframe();
    }

    // Set uniforms
    setTransformUniforms(*wireframeProgram);
    wireframeProgram->setUniform("u_edgeWidth", getEdgeWidth() * render::engine->getCurrentPixelScaling());
    wireframeProgram->setUniform("u_edgeColor", getEdgeColor());

    // Make sure wireframe wins depth tests
    render::engine->setDepthMode(DepthMode::LEqualReadOnly);

    // slightly weird blend function: ensures alpha is set by whatever was drawn before, rather than the wireframe
    render::engine->setBlendMode(BlendMode::OverNoWrite);

    wireframeProgram->draw();

    // Return to default modes
    render::engine->setBlendMode();
    render::engine->setDepthMode();
  }
}

void SurfaceMesh::drawPick() {
  if (!isEnabled()) {
    return;
  }

  if (pickProgram == nullptr) {
    preparePick();
  }

  // Set uniforms
  setTransformUniforms(*pickProgram);

  pickProgram->draw();
}

void SurfaceMesh::prepare() {
  program = render::engine->generateShaderProgram(
      {render::PLAIN_SURFACE_VERT_SHADER, render::PLAIN_SURFACE_FRAG_SHADER}, DrawMode::Triangles);

  // Populate draw buffers
  fillGeometryBuffers(*program);
  render::engine->setMaterial(*program, getMaterial());
}

void SurfaceMesh::prepareWireframe() {
  wireframeProgram = render::engine->generateShaderProgram(
      {render::SURFACE_WIREFRAME_VERT_SHADER, render::SURFACE_WIREFRAME_FRAG_SHADER}, DrawMode::Triangles);

  // Populate draw buffers
  fillGeometryBuffersWireframe(*wireframeProgram);
  render::engine->setMaterial(*wireframeProgram, getMaterial());
}

void SurfaceMesh::preparePick() {

  // Create a new program
  pickProgram = render::engine->generateShaderProgram(
      {render::PICK_SURFACE_VERT_SHADER, render::PICK_SURFACE_FRAG_SHADER}, DrawMode::Triangles);

  // Get element indices
  size_t totalPickElements = nVertices() + nFaces() + nEdges() + nHalfedges();

  // In "local" indices, indexing elements only within this triMesh, used for reading later
  facePickIndStart = nVertices();
  edgePickIndStart = facePickIndStart + nFaces();
  halfedgePickIndStart = edgePickIndStart + nEdges();

  // In "global" indices, indexing all elements in the scene, used to fill buffers for drawing here
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);
  size_t faceGlobalPickIndStart = pickStart + nVertices();
  size_t edgeGlobalPickIndStart = faceGlobalPickIndStart + nFaces();
  size_t halfedgeGlobalPickIndStart = edgeGlobalPickIndStart + nEdges();

  // == Fill buffers
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> bcoord;
  std::vector<std::array<glm::vec3, 3>> vertexColors, edgeColors, halfedgeColors;
  std::vector<glm::vec3> faceColor;

  // Reserve space
  positions.reserve(3 * nFacesTriangulation());
  bcoord.reserve(3 * nFacesTriangulation());
  vertexColors.reserve(3 * nFacesTriangulation());
  edgeColors.reserve(3 * nFacesTriangulation());
  halfedgeColors.reserve(3 * nFacesTriangulation());
  faceColor.reserve(3 * nFacesTriangulation());

  // Build all quantities in each face
  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    glm::vec3 pRoot = vertices[vRoot];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      glm::vec3 pB = vertices[vB];
      size_t vC = face[(j + 1) % D];
      glm::vec3 pC = vertices[vC];

      glm::vec3 fColor = pick::indToVec(iF + faceGlobalPickIndStart);
      std::array<size_t, 3> vertexInds = {vRoot, vB, vC};

      positions.push_back(pRoot);
      positions.push_back(pB);
      positions.push_back(pC);

      // Build all quantities
      std::array<glm::vec3, 3> vColor;

      for (size_t i = 0; i < 3; i++) {
        // Want just one copy of face color, so we can build it in the usual way
        faceColor.push_back(fColor);

        // Vertex index color
        vColor[i] = pick::indToVec(vertexInds[i] + pickStart);
      }

      std::array<glm::vec3, 3> eColor = {fColor, pick::indToVec(edgeIndices[iF][j] + edgeGlobalPickIndStart), fColor};
      std::array<glm::vec3, 3> heColor = {fColor, pick::indToVec(halfedgeIndices[iF][j] + halfedgeGlobalPickIndStart),
                                          fColor};

      // First edge is a real edge
      if (j == 1) {
        eColor[0] = pick::indToVec(edgeIndices[iF][0] + edgeGlobalPickIndStart);
        heColor[0] = pick::indToVec(halfedgeIndices[iF][0] + halfedgeGlobalPickIndStart);
      }
      // Last is a real edge
      if (j + 2 == D) {
        eColor[2] = pick::indToVec(edgeIndices[iF].back() + edgeGlobalPickIndStart);
        heColor[2] = pick::indToVec(halfedgeIndices[iF].back() + halfedgeGlobalPickIndStart);
      }

      // Push three copies of the values needed at each vertex
      for (int j = 0; j < 3; j++) {
        vertexColors.push_back(vColor);
        edgeColors.push_back(eColor);
        halfedgeColors.push_back(heColor);
      }

      // Barycoords
      bcoord.push_back(glm::vec3{1.0, 0.0, 0.0});
      bcoord.push_back(glm::vec3{0.0, 1.0, 0.0});
      bcoord.push_back(glm::vec3{0.0, 0.0, 1.0});
    }
  }

  // Store data in buffers
  pickProgram->setAttribute("a_position", positions);
  pickProgram->setAttribute("a_barycoord", bcoord);
  pickProgram->setAttribute<glm::vec3, 3>("a_vertexColors", vertexColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_edgeColors", edgeColors);
  pickProgram->setAttribute<glm::vec3, 3>("a_halfedgeColors", halfedgeColors);
  pickProgram->setAttribute("a_faceColor", faceColor);
}

void SurfaceMesh::fillGeometryBuffers(render::ShaderProgram& p) {
  if (isSmoothShade()) {
    fillGeometryBuffersSmooth(p);
  } else {
    fillGeometryBuffersFlat(p);
  }
}

void SurfaceMesh::fillGeometryBuffersSmooth(render::ShaderProgram& p) {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;

  bool wantsBary = p.hasAttribute("a_barycoord");

  // Reserve space
  positions.reserve(3 * nFacesTriangulation());
  normals.reserve(3 * nFacesTriangulation());
  if (wantsBary) {
    bcoord.reserve(3 * nFacesTriangulation());
  }

  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    glm::vec3 pRoot = vertices[vRoot];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      glm::vec3 pB = vertices[vB];
      size_t vC = face[(j + 1) % D];
      glm::vec3 pC = vertices[vC];

      positions.push_back(pRoot);
      positions.push_back(pB);
      positions.push_back(pC);

      normals.push_back(vertexNormals[vRoot]);
      normals.push_back(vertexNormals[vB]);
      normals.push_back(vertexNormals[vC]);

      if (wantsBary) {
        bcoord.push_back(glm::vec3{1., 0., 0.});
        bcoord.push_back(glm::vec3{0., 1., 0.});
        bcoord.push_back(glm::vec3{0., 0., 1.});
      }
    }
  }

  // Store data in buffers
  p.setAttribute("a_position", positions);
  p.setAttribute("a_normal", normals);
  if (wantsBary) {
    p.setAttribute("a_barycoord", bcoord);
  }
}

void SurfaceMesh::fillGeometryBuffersFlat(render::ShaderProgram& p) {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;

  bool wantsBary = p.hasAttribute("a_barycoord");

  positions.reserve(3 * nFacesTriangulation());
  normals.reserve(3 * nFacesTriangulation());
  if (wantsBary) {
    bcoord.reserve(3 * nFacesTriangulation());
  }

  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();
    glm::vec3 faceN = faceNormals[iF];

    // implicitly triangulate from root
    size_t vRoot = face[0];
    glm::vec3 pRoot = vertices[vRoot];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      glm::vec3 pB = vertices[vB];
      size_t vC = face[(j + 1) % D];
      glm::vec3 pC = vertices[vC];

      positions.push_back(pRoot);
      positions.push_back(pB);
      positions.push_back(pC);

      normals.push_back(faceN);
      normals.push_back(faceN);
      normals.push_back(faceN);

      if (wantsBary) {
        bcoord.push_back(glm::vec3{1., 0., 0.});
        bcoord.push_back(glm::vec3{0., 1., 0.});
        bcoord.push_back(glm::vec3{0., 0., 1.});
      }
    }
  }


  // Store data in buffers
  p.setAttribute("a_position", positions);
  p.setAttribute("a_normal", normals);
  if (wantsBary) {
    p.setAttribute("a_barycoord", bcoord);
  }
}

void SurfaceMesh::fillGeometryBuffersWireframe(render::ShaderProgram& p) {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> bcoord;
  std::vector<glm::vec3> edgeReal;

  positions.reserve(3 * nFacesTriangulation());
  normals.reserve(3 * nFacesTriangulation());
  bcoord.reserve(3 * nFacesTriangulation());
  edgeReal.reserve(3 * nFacesTriangulation());

  for (size_t iF = 0; iF < nFaces(); iF++) {
    auto& face = faces[iF];
    size_t D = face.size();
    glm::vec3 faceN = faceNormals[iF];

    // implicitly triangulate from root
    size_t vRoot = face[0];
    glm::vec3 pRoot = vertices[vRoot];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      glm::vec3 pB = vertices[vB];
      size_t vC = face[(j + 1) % D];
      glm::vec3 pC = vertices[vC];

      positions.push_back(pRoot);
      positions.push_back(pB);
      positions.push_back(pC);

      normals.push_back(faceN);
      normals.push_back(faceN);
      normals.push_back(faceN);

      bcoord.push_back(glm::vec3{1., 0., 0.});
      bcoord.push_back(glm::vec3{0., 1., 0.});
      bcoord.push_back(glm::vec3{0., 0., 1.});

      glm::vec3 edgeRealV{0., 1., 0.};
      if (j == 1) {
        edgeRealV.x = 1.;
      }
      if (j + 2 == D) {
        edgeRealV.z = 1.;
      }
      edgeReal.push_back(edgeRealV);
      edgeReal.push_back(edgeRealV);
      edgeReal.push_back(edgeRealV);
    }
  }


  // Store data in buffers
  p.setAttribute("a_position", positions);
  p.setAttribute("a_normal", normals);
  p.setAttribute("a_barycoord", bcoord);
  p.setAttribute("a_edgeReal", edgeReal);
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


void SurfaceMesh::buildVertexInfoGui(size_t vInd) {

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

void SurfaceMesh::buildFaceInfoGui(size_t fInd) {
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

  { // colors
    if (ImGui::ColorEdit3("Color", &surfaceColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setSurfaceColor(surfaceColor.get());
    ImGui::SameLine();
  }

  { // Flat shading or smooth shading?
    ImGui::SameLine();
    if (ImGui::Checkbox("Smooth", &shadeSmooth.get())) setSmoothShade(shadeSmooth.get());
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
      if (ImGui::SliderFloat("Width", &edgeWidth.get(), 0.001, 2.)) setEdgeWidth(getEdgeWidth());
      ImGui::PopItemWidth();
    }
    ImGui::PopItemWidth();
  }
}

void SurfaceMesh::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}


void SurfaceMesh::geometryChanged() {
  program.reset();
  wireframeProgram.reset();
  pickProgram.reset();

  computeGeometryData();

  for (auto& q : quantities) {
    q.second->geometryChanged();
  }

  requestRedraw();
}

double SurfaceMesh::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (glm::vec3 p : vertices) {
    glm::vec3 transPos = glm::vec3(objectTransform * glm::vec4(p.x, p.y, p.z, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(transPos - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> SurfaceMesh::boundingBox() {
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3 pOrig : vertices) {
    glm::vec3 p = glm::vec3(objectTransform * glm::vec4(pOrig, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

std::string SurfaceMesh::typeName() { return structureTypeName; }

long long int SurfaceMesh::selectVertex() {

  // Make sure we can see edges
  edgeWidth = 1.;
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

  return returnVertInd;
}

/*
FacePtr SurfaceMesh::selectFace() {

  // Make sure we can see edges
  edgeWidth = 1.;
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

// === Option getters and setters

SurfaceMesh* SurfaceMesh::setSmoothShade(bool isSmooth) {
  shadeSmooth = isSmooth;
  geometryChanged();
  requestRedraw();
  return this;
}
bool SurfaceMesh::isSmoothShade() { return shadeSmooth.get(); }

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
  geometryChanged(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
  requestRedraw();
  return this;
}
std::string SurfaceMesh::getMaterial() { return material.get(); }

SurfaceMesh* SurfaceMesh::setEdgeWidth(double newVal) {
  edgeWidth = newVal;
  requestRedraw();
  return this;
}
double SurfaceMesh::getEdgeWidth() { return edgeWidth.get(); }

// === Quantity adders


SurfaceVertexColorQuantity* SurfaceMesh::addVertexColorQuantityImpl(std::string name,
                                                                    const std::vector<glm::vec3>& colors) {
  SurfaceVertexColorQuantity* q = new SurfaceVertexColorQuantity(name, applyPermutation(colors, vertexPerm), *this);
  addQuantity(q);
  return q;
}

SurfaceFaceColorQuantity* SurfaceMesh::addFaceColorQuantityImpl(std::string name,
                                                                const std::vector<glm::vec3>& colors) {
  SurfaceFaceColorQuantity* q = new SurfaceFaceColorQuantity(name, applyPermutation(colors, facePerm), *this);
  addQuantity(q);
  return q;
}

SurfaceDistanceQuantity* SurfaceMesh::addVertexDistanceQuantityImpl(std::string name, const std::vector<double>& data) {
  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(name, applyPermutation(data, vertexPerm), *this, false);
  addQuantity(q);
  return q;
}

SurfaceDistanceQuantity* SurfaceMesh::addVertexSignedDistanceQuantityImpl(std::string name,
                                                                          const std::vector<double>& data) {
  SurfaceDistanceQuantity* q = new SurfaceDistanceQuantity(name, applyPermutation(data, vertexPerm), *this, true);
  addQuantity(q);
  return q;
}

SurfaceCornerParameterizationQuantity*
SurfaceMesh::addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                             ParamCoordsType type) {
  SurfaceCornerParameterizationQuantity* q = new SurfaceCornerParameterizationQuantity(
      name, applyPermutation(coords, cornerPerm), type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);

  return q;
}

SurfaceVertexParameterizationQuantity*
SurfaceMesh::addVertexParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                   ParamCoordsType type) {
  SurfaceVertexParameterizationQuantity* q = new SurfaceVertexParameterizationQuantity(
      name, applyPermutation(coords, vertexPerm), type, ParamVizStyle::CHECKER, *this);
  addQuantity(q);

  return q;
}

SurfaceVertexParameterizationQuantity*
SurfaceMesh::addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords,
                                                  ParamCoordsType type) {
  SurfaceVertexParameterizationQuantity* q = new SurfaceVertexParameterizationQuantity(
      name, applyPermutation(coords, vertexPerm), type, ParamVizStyle::LOCAL_CHECK, *this);
  addQuantity(q);

  return q;
}


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


SurfaceVertexScalarQuantity* SurfaceMesh::addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                      DataType type) {
  SurfaceVertexScalarQuantity* q =
      new SurfaceVertexScalarQuantity(name, applyPermutation(data, vertexPerm), *this, type);
  addQuantity(q);
  return q;
}

SurfaceFaceScalarQuantity* SurfaceMesh::addFaceScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                  DataType type) {
  SurfaceFaceScalarQuantity* q = new SurfaceFaceScalarQuantity(name, applyPermutation(data, facePerm), *this, type);
  addQuantity(q);
  return q;
}


SurfaceEdgeScalarQuantity* SurfaceMesh::addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                  DataType type) {
  SurfaceEdgeScalarQuantity* q = new SurfaceEdgeScalarQuantity(name, applyPermutation(data, edgePerm), *this, type);
  addQuantity(q);
  return q;
}

SurfaceHalfedgeScalarQuantity*
SurfaceMesh::addHalfedgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  SurfaceHalfedgeScalarQuantity* q =
      new SurfaceHalfedgeScalarQuantity(name, applyPermutation(data, halfedgePerm), *this, type);
  addQuantity(q);
  return q;
}


SurfaceVertexVectorQuantity* SurfaceMesh::addVertexVectorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& vectors,
                                                                      VectorType vectorType) {
  SurfaceVertexVectorQuantity* q =
      new SurfaceVertexVectorQuantity(name, applyPermutation(vectors, vertexPerm), *this, vectorType);
  addQuantity(q);
  return q;
}

SurfaceFaceVectorQuantity*
SurfaceMesh::addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType) {

  SurfaceFaceVectorQuantity* q =
      new SurfaceFaceVectorQuantity(name, applyPermutation(vectors, facePerm), *this, vectorType);
  addQuantity(q);
  return q;
}

SurfaceFaceIntrinsicVectorQuantity*
SurfaceMesh::addFaceIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, int nSym,
                                                VectorType vectorType) {

  SurfaceFaceIntrinsicVectorQuantity* q =
      new SurfaceFaceIntrinsicVectorQuantity(name, applyPermutation(vectors, facePerm), *this, nSym, vectorType);
  addQuantity(q);
  return q;
}


SurfaceVertexIntrinsicVectorQuantity*
SurfaceMesh::addVertexIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, int nSym,
                                                  VectorType vectorType) {
  SurfaceVertexIntrinsicVectorQuantity* q =
      new SurfaceVertexIntrinsicVectorQuantity(name, applyPermutation(vectors, vertexPerm), *this, nSym, vectorType);
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


void SurfaceMesh::setVertexTangentBasisXImpl(const std::vector<glm::vec3>& vectors) {

  std::vector<glm::vec3> inputBasisX = applyPermutation(vectors, vertexPerm);
  vertexTangentSpaces.resize(nVertices());

  for (size_t iV = 0; iV < nVertices(); iV++) {

    glm::vec3 basisX = inputBasisX[iV];
    glm::vec3 normal = vertexNormals[iV];

    // Project in to tangent defined by our normals
    basisX = glm::normalize(basisX - normal * glm::dot(normal, basisX));

    // Let basis Y complete the frame
    glm::vec3 basisY = glm::cross(normal, basisX);

    vertexTangentSpaces[iV][0] = basisX;
    vertexTangentSpaces[iV][1] = basisY;
  }
}

void SurfaceMesh::setFaceTangentBasisXImpl(const std::vector<glm::vec3>& vectors) {

  std::vector<glm::vec3> inputBasisX = applyPermutation(vectors, facePerm);
  faceTangentSpaces.resize(nFaces());

  for (size_t iF = 0; iF < nFaces(); iF++) {

    glm::vec3 basisX = inputBasisX[iF];
    glm::vec3 normal = faceNormals[iF];

    // Project in to tangent defined by our normals
    basisX = glm::normalize(basisX - normal * glm::dot(normal, basisX));

    // Let basis Y complete the frame
    glm::vec3 basisY = glm::cross(normal, basisX);

    faceTangentSpaces[iF][0] = basisX;
    faceTangentSpaces[iF][1] = basisY;
  }
}


SurfaceMeshQuantity::SurfaceMeshQuantity(std::string name, SurfaceMesh& parentStructure, bool dominates)
    : Quantity<SurfaceMesh>(name, parentStructure, dominates) {}
void SurfaceMeshQuantity::geometryChanged() {}
void SurfaceMeshQuantity::buildVertexInfoGUI(size_t vInd) {}
void SurfaceMeshQuantity::buildFaceInfoGUI(size_t fInd) {}
void SurfaceMeshQuantity::buildEdgeInfoGUI(size_t eInd) {}
void SurfaceMeshQuantity::buildHalfedgeInfoGUI(size_t heInd) {}

} // namespace polyscope

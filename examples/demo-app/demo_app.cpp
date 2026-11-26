#include "polyscope/polyscope.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/messages.h"

#include "polyscope/camera_view.h"
#include "polyscope/curve_network.h"
#include "polyscope/file_helpers.h"
#include "polyscope/floating_quantity_structure.h"
#include "polyscope/implicit_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/simple_triangle_mesh.h"
#include "polyscope/sparse_volume_grid.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/types.h"
#include "polyscope/view.h"
#include "polyscope/volume_grid.h"
#include "polyscope/volume_mesh.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <unordered_set>
#include <utility>

#include "args/args.hxx"
#include "happly.h"
#include "nlohmann/json.hpp"

#include "simple_dot_mesh_parser.h"
#include "surface_mesh_io.h"

#include "glm/gtx/string_cast.hpp"

#include "stb_image.h"


bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void constructDemoCurveNetwork(std::string curveName, std::vector<glm::vec3> nodes,
                               std::vector<std::array<size_t, 2>> edges) {

  // Add the curve
  if (edges.size() > 0) {
    polyscope::registerCurveNetwork(curveName, nodes, edges);
  }

  // Useful data
  size_t nNodes = nodes.size();
  size_t nEdges = edges.size();

  { // Add some node values
    std::vector<double> valX(nNodes);
    std::vector<double> valNodeCat(nNodes);
    std::vector<double> valXabs(nNodes);
    std::vector<std::array<double, 3>> randColor(nNodes);
    std::vector<glm::vec3> randVec(nNodes);
    for (size_t iN = 0; iN < nNodes; iN++) {
      valX[iN] = nodes[iN].x;
      valNodeCat[iN] = iN * 5 / nNodes;
      valXabs[iN] = std::fabs(nodes[iN].x);
      randColor[iN] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iN] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
    }
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("nX", valX);
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("nXabs", valXabs);
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("node categorical", valNodeCat,
                                                                 polyscope::DataType::CATEGORICAL);
    polyscope::getCurveNetwork(curveName)->addNodeColorQuantity("nColor", randColor);
    polyscope::getCurveNetwork(curveName)->addNodeVectorQuantity("randVecN", randVec);
  }

  { // Add some edge values
    std::vector<double> edgeLen(nEdges);
    std::vector<double> valEdgeCat(nEdges);
    std::vector<std::array<double, 3>> randColor(nEdges);
    std::vector<glm::vec3> randVec(nEdges);
    std::vector<double> valYabs(nEdges);
    for (size_t iE = 0; iE < nEdges; iE++) {
      auto edge = edges[iE];
      size_t nA = std::get<0>(edge);
      size_t nB = std::get<1>(edge);
      glm::vec3 posA = nodes[nA];
      glm::vec3 posB = nodes[nB];
      glm::vec3 midpointPos = 0.5f * (posA + posB);

      edgeLen[iE] = glm::length(nodes[nA] - nodes[nB]);
      valEdgeCat[iE] = iE * 5 / nEdges;
      randColor[iE] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iE] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
      valYabs[iE] = std::fabs(midpointPos.y);
    }
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge len", edgeLen, polyscope::DataType::MAGNITUDE);
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge valYabs", valYabs,
                                                                 polyscope::DataType::MAGNITUDE);
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge categorical", valEdgeCat,
                                                                 polyscope::DataType::CATEGORICAL);
    polyscope::getCurveNetwork(curveName)->addEdgeColorQuantity("eColor", randColor);
    polyscope::getCurveNetwork(curveName)->addEdgeVectorQuantity("randVecE", randVec);
  }

  // set a node radius quantity from above
  polyscope::getCurveNetwork(curveName)->setNodeRadiusQuantity("nXabs");
  polyscope::getCurveNetwork(curveName)->setEdgeRadiusQuantity("edge valYabs");
}

void processFileOBJ(std::string filename) {
  // Get a nice name for the file
  std::string niceName = polyscope::guessNiceNameFromPath(filename);

  // Load mesh and polygon soup data
  std::vector<std::array<double, 3>> vertexPositions;
  std::vector<std::vector<size_t>> faceIndices;
  polyscope::loadPolygonSoup_OBJ(filename, vertexPositions, faceIndices);
  std::vector<glm::vec3> vertexPositionsGLM;
  for (std::array<double, 3> p : vertexPositions) {
    vertexPositionsGLM.push_back(glm::vec3{p[0], p[1], p[2]});
  }
  auto psMesh = polyscope::registerSurfaceMesh(niceName, vertexPositionsGLM, faceIndices);

  auto psSimpleMesh = polyscope::registerSimpleTriangleMesh(niceName + " (simple)", vertexPositionsGLM, faceIndices);
  psSimpleMesh->setEnabled(false);

  // Useful data
  size_t nVertices = psMesh->nVertices();
  size_t nFaces = psMesh->nFaces();

  // Add some vertex scalars
  std::vector<double> valX(nVertices);
  std::vector<double> valY(nVertices);
  std::vector<double> valZ(nVertices);
  std::vector<double> valMag(nVertices);
  std::vector<double> valCat(nVertices);
  std::vector<std::array<double, 3>> randColor(nVertices);
  for (size_t iV = 0; iV < nVertices; iV++) {
    valX[iV] = vertexPositionsGLM[iV].x / 10000;
    valY[iV] = vertexPositionsGLM[iV].y;
    valZ[iV] = vertexPositionsGLM[iV].z;
    valMag[iV] = glm::length(vertexPositionsGLM[iV]);
    valCat[iV] = (int32_t)(iV * 7 / nVertices) - 2;

    randColor[iV] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
  }
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cX_really_really_stupid_long_name_how_dumb", valX);
  auto q = polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY", valY);
  q->setOnscreenColorbarEnabled(true); // set the onscreen colormap for this one
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cZ", valZ);
  polyscope::getSurfaceMesh(niceName)->addVertexColorQuantity("vColor", randColor);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY_sym", valY, polyscope::DataType::SYMMETRIC);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cNorm", valMag, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("categorical vert", valCat,
                                                               polyscope::DataType::CATEGORICAL);

  // Simple triangle mesh quantities
  psSimpleMesh->addVertexScalarQuantity("cY", valY);
  psSimpleMesh->addVertexColorQuantity("vColor", randColor);

  polyscope::getSurfaceMesh(niceName)->addVertexDistanceQuantity("cY_dist", valY);
  polyscope::getSurfaceMesh(niceName)->addVertexSignedDistanceQuantity("cY_signeddist", valY);


  // Add some face scalars
  std::vector<double> fArea(nFaces);
  std::vector<double> zero(nFaces);
  std::vector<double> fCat(nFaces);
  std::vector<std::array<double, 3>> fColor(nFaces);
  for (size_t iF = 0; iF < nFaces; iF++) {
    std::vector<size_t>& face = faceIndices[iF];

    // Compute something like area
    double area = 0;
    for (size_t iV = 1; iV < face.size() - 1; iV++) {
      glm::vec3 p0 = vertexPositionsGLM[face[0]];
      glm::vec3 p1 = vertexPositionsGLM[face[iV]];
      glm::vec3 p2 = vertexPositionsGLM[face[iV + 1]];
      area += 0.5f * glm::length(glm::cross(p1 - p0, p2 - p0));
    }
    fArea[iF] = area;

    zero[iF] = 0;
    fColor[iF] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
    fCat[iF] = (int32_t)(iF * 25 / nFaces) - 12;
  }
  polyscope::getSurfaceMesh(niceName)->addFaceScalarQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addFaceScalarQuantity("zero", zero);
  polyscope::getSurfaceMesh(niceName)->addFaceColorQuantity("fColor", fColor);
  polyscope::getSurfaceMesh(niceName)->addFaceScalarQuantity("categorical face", fCat,
                                                             polyscope::DataType::CATEGORICAL);

  psSimpleMesh->addFaceScalarQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);
  psSimpleMesh->addFaceColorQuantity("fColor", fColor);


  // size_t nEdges = psMesh->nEdges();

  // Edge/halfedge/corner data
  std::vector<double> eLen;
  std::vector<double> heLen;
  std::vector<double> cAngle;
  std::vector<double> cID;
  std::vector<double> eCat;
  std::vector<double> heCat;
  std::vector<double> cCat;
  std::unordered_set<std::pair<size_t, size_t>, polyscope::hash_combine::hash<std::pair<size_t, size_t>>> seenEdges;
  std::vector<uint32_t> edgeOrdering;
  for (size_t iF = 0; iF < nFaces; iF++) {
    std::vector<size_t>& face = faceIndices[iF];

    for (size_t iC = 0; iC < face.size(); iC++) {
      size_t i0 = face[iC];
      size_t i1 = face[(iC + 1) % face.size()];
      size_t im1 = face[(iC + face.size() - 1) % face.size()];
      glm::vec3 p0 = vertexPositionsGLM[i0];
      glm::vec3 p1 = vertexPositionsGLM[i1];
      glm::vec3 pm1 = vertexPositionsGLM[im1];

      double len = glm::length(p0 - p1);

      double angle = glm::acos(glm::dot(glm::normalize(p1 - p0), glm::normalize(pm1 - p0)));

      size_t iMin = std::min(i0, i1);
      size_t iMax = std::max(i0, i1);

      auto p = std::make_pair(iMin, iMax);
      if (seenEdges.find(p) == seenEdges.end()) {
        eLen.push_back(len);
        eCat.push_back((iF + iC) % 5);
        edgeOrdering.push_back(edgeOrdering.size()); // totally coincidentally, this is the trivial ordering
        seenEdges.insert(p);
      }
      heLen.push_back(len);
      cAngle.push_back(angle);
      heCat.push_back((iF + iC) % 7);
      cCat.push_back(i0 % 12);
      cID.push_back(iC);
    }
  }
  size_t nEdges = edgeOrdering.size();
  polyscope::getSurfaceMesh(niceName)->setEdgePermutation(edgeOrdering);
  polyscope::getSurfaceMesh(niceName)->addEdgeScalarQuantity("edge length", eLen);
  polyscope::getSurfaceMesh(niceName)->addHalfedgeScalarQuantity("halfedge length", heLen);
  polyscope::getSurfaceMesh(niceName)->addCornerScalarQuantity("corner angle", cAngle);
  polyscope::getSurfaceMesh(niceName)->addCornerScalarQuantity("corner ID", cID);
  polyscope::getSurfaceMesh(niceName)->addEdgeScalarQuantity("categorical edge", eCat,
                                                             polyscope::DataType::CATEGORICAL);
  polyscope::getSurfaceMesh(niceName)->addHalfedgeScalarQuantity("categorical halfedge", heCat,
                                                                 polyscope::DataType::CATEGORICAL);
  polyscope::getSurfaceMesh(niceName)->addCornerScalarQuantity("categorical corner", cCat,
                                                               polyscope::DataType::CATEGORICAL);


  // Test error
  /*
  polyscope::error("Resistance is futile.");
  polyscope::error("I'm a really, really, frustrating long error. What are you going to do with me? How ever will we "
               "share this crisis in a way which looks right while properly wrapping text in some form or other?");
  polyscope::terminatingError("and that was all");

  // Test warning
  polyscope::warning("Something went slightly wrong", "it was bad");

  polyscope::warning("Something else went slightly wrong", "it was also bad");
  polyscope::warning("Something went slightly wrong", "it was still bad");
  for (int i = 0; i < 5000; i++) {
  polyscope::warning("Some problems come in groups", "detail = " + std::to_string(i));
  }
  */

  // === Add some vectors

  // Face & vertex normals
  std::vector<glm::vec3> fNormals(nFaces);
  std::vector<glm::vec3> fCenters(nFaces);
  std::vector<glm::vec3> vNormals(nVertices, glm::vec3{0., 0., 0.});
  for (size_t iF = 0; iF < nFaces; iF++) {
    std::vector<size_t>& face = faceIndices[iF];

    // Compute a center (used below)
    glm::vec3 C = {0., 0., 0.};
    for (size_t iV = 0; iV < face.size(); iV++) {
      C += vertexPositionsGLM[face[iV]];
    }
    C /= face.size();
    fCenters[iF] = C;

    // Compute something like a normal
    glm::vec3 N = {0., 0., 0.};
    for (size_t iV = 1; iV < face.size() - 1; iV++) {
      glm::vec3 p0 = vertexPositionsGLM[face[0]];
      glm::vec3 p1 = vertexPositionsGLM[face[iV]];
      glm::vec3 p2 = vertexPositionsGLM[face[iV + 1]];
      N += glm::cross(p1 - p0, p2 - p0);
    }
    N = glm::normalize(N);
    fNormals[iF] = N;

    // Accumulate at vertices
    for (size_t iV = 0; iV < face.size(); iV++) {
      vNormals[face[iV]] += N;
    }
  }
  polyscope::getSurfaceMesh(niceName)->addFaceVectorQuantity("face normals", fNormals);


  std::vector<glm::vec3> vNormalsRand(nVertices, glm::vec3{0., 0., 0.});
  std::vector<glm::vec3> toZero(nVertices, glm::vec3{0., 0., 0.});
  for (size_t iV = 0; iV < nVertices; iV++) {
    vNormals[iV] = glm::normalize(vNormals[iV]);
    vNormalsRand[iV] = vNormals[iV] * (float)polyscope::randomUnit() * 5000.f;
    toZero[iV] = -vertexPositionsGLM[iV];
  }

  polyscope::getSurfaceMesh(niceName)->addVertexVectorQuantity("area vertex normals", vNormals);
  polyscope::getSurfaceMesh(niceName)->addVertexVectorQuantity("rand length normals", vNormalsRand);
  polyscope::getSurfaceMesh(niceName)->addVertexVectorQuantity("toZero", toZero, polyscope::VectorType::AMBIENT);


  { // Some kind of intrinsic vector field
    // Project this weird swirly field on to the surface (the ABC flow)
    auto spatialFunc = [&](glm::vec3 p) {
      float A = 1.;
      float B = 1.;
      float C = 1.;
      float xComp = A * std::sin(p.z) + C * std::cos(p.y);
      float yComp = B * std::sin(p.x) + A * std::cos(p.z);
      float zComp = C * std::sin(p.y) + B * std::cos(p.x);
      return glm::vec3{xComp, yComp, zComp};
    };

    auto constructBasis = [&](glm::vec3 unitNormal) -> std::tuple<glm::vec3, glm::vec3> {
      glm::vec3 basisX{1., 0., 0.};
      basisX -= dot(basisX, unitNormal) * unitNormal;
      if (std::abs(basisX.x) < 0.1) {
        basisX = glm::vec3{0., 1., 0.};
        basisX -= glm::dot(basisX, unitNormal) * unitNormal;
      }
      basisX = glm::normalize(basisX);
      glm::vec3 basisY = glm::normalize(glm::cross(unitNormal, basisX));
      return std::make_tuple(basisX, basisY);
    };

    // vertex tangent bases
    std::vector<glm::vec3> vertexBasisX(nVertices);
    std::vector<glm::vec3> vertexBasisY(nVertices);
    for (size_t i = 0; i < nVertices; i++) {
      std::tie(vertexBasisX[i], vertexBasisY[i]) = constructBasis(vNormals[i]);
    }

    // face tangent bases
    std::vector<glm::vec3> faceBasisX(nFaces);
    std::vector<glm::vec3> faceBasisY(nFaces);
    for (size_t i = 0; i < nFaces; i++) {
      std::tie(faceBasisX[i], faceBasisY[i]) = constructBasis(fNormals[i]);
    }

    // At vertices
    std::vector<glm::vec2> vertexTangentVec(nVertices, glm::vec3{0., 0., 0.});
    for (size_t iV = 0; iV < nVertices; iV++) {
      glm::vec3 pos = vertexPositionsGLM[iV];
      glm::vec3 basisX = vertexBasisX[iV];
      glm::vec3 basisY = vertexBasisY[iV];

      glm::vec3 v = spatialFunc(pos);
      glm::vec2 vTangent{glm::dot(v, basisX), glm::dot(v, basisY)};
      vertexTangentVec[iV] = vTangent;
    }
    psMesh->addVertexTangentVectorQuantity("tangent vertex vec", vertexTangentVec, vertexBasisX, vertexBasisY);
    psMesh->addVertexTangentVectorQuantity("tangent vertex vec line", vertexTangentVec, vertexBasisX, vertexBasisY, 2);

    // At faces
    std::vector<glm::vec2> faceTangentVec(nFaces, glm::vec3{0., 0., 0.});
    for (size_t iF = 0; iF < nFaces; iF++) {

      glm::vec3 pos = fCenters[iF];
      glm::vec3 basisX = faceBasisX[iF];
      glm::vec3 basisY = faceBasisY[iF];

      glm::vec3 v = spatialFunc(pos);
      glm::vec2 vTangent{glm::dot(v, basisX), glm::dot(v, basisY)};
      faceTangentVec[iF] = vTangent;
    }
    psMesh->addFaceTangentVectorQuantity("tangent face vec", faceTangentVec, faceBasisX, faceBasisY);
    psMesh->addFaceTangentVectorQuantity("tangent face vec cross", faceTangentVec, faceBasisX, faceBasisY, 4);


    // 1-form
    std::vector<double> edgeForm(nEdges, 0.);
    std::vector<char> edgeOrient(nEdges, false);
    bool isTriangle = true;
    size_t iEdge = 0;
    seenEdges.clear();
    for (size_t iF = 0; iF < nFaces; iF++) {
      std::vector<size_t>& face = faceIndices[iF];

      if (face.size() != 3) {
        isTriangle = false;
        break;
      }

      glm::vec3 pos = fCenters[iF];

      for (size_t j = 0; j < face.size(); j++) {
        size_t vA = face[j];
        size_t vB = face[(j + 1) % face.size()];
        size_t iMin = std::min(vA, vB);
        size_t iMax = std::max(vA, vB);
        auto p = std::make_pair(iMin, iMax);
        if (seenEdges.find(p) == seenEdges.end()) { // use the hashset again to iterate over edges in order
          glm::vec3 v = spatialFunc(pos);
          glm::vec3 edgeVec = vertexPositionsGLM[vB] - vertexPositionsGLM[vA];
          edgeForm[iEdge] = glm::dot(edgeVec, v);
          edgeOrient[iEdge] = (vB > vA);
          seenEdges.insert(p);
          iEdge++;
        }
      }
    }
    if (isTriangle) {
      psMesh->addOneFormTangentVectorQuantity("intrinsic 1-form", edgeForm, edgeOrient);
    }
  }

  { // Parameterizations
    std::vector<std::array<double, 2>> cornerParam;
    for (size_t iF = 0; iF < nFaces; iF++) {
      std::vector<size_t>& face = faceIndices[iF];
      for (size_t iC = 0; iC < face.size(); iC++) {
        size_t iV = face[iC];
        std::array<double, 2> p = {{vertexPositionsGLM[iV].x, vertexPositionsGLM[iV].y}};
        cornerParam.push_back(p);
      }
    }
    polyscope::getSurfaceMesh(niceName)->addParameterizationQuantity("param test", cornerParam);


    std::vector<std::array<double, 2>> vertParam;
    for (size_t iV = 0; iV < nVertices; iV++) {
      std::array<double, 2> p = {{vertexPositionsGLM[iV].x, vertexPositionsGLM[iV].y}};
      vertParam.push_back(p);
    }
    polyscope::getSurfaceMesh(niceName)->addVertexParameterizationQuantity("param vert test", vertParam);


    // local param about vert
    std::vector<std::array<double, 2>> vertParamLocal;
    size_t iCenter = nVertices / 2;
    glm::vec3 cP = vertexPositionsGLM[iCenter];
    glm::vec3 cN = vNormals[iCenter];

    // make a basis
    glm::vec3 basisX{0.1234, -0.98823, .33333}; // provably random
    basisX = basisX - glm::dot(cN, basisX) * cN;
    basisX = glm::normalize(basisX);
    glm::vec3 basisY = -glm::cross(basisX, cN);

    for (size_t iV = 0; iV < nVertices; iV++) {

      glm::vec3 vec = vertexPositionsGLM[iV] - cP;

      std::array<double, 2> p = {{glm::dot(basisX, vec), glm::dot(basisY, vec)}};
      vertParamLocal.push_back(p);
    }

    polyscope::getSurfaceMesh(niceName)->addLocalParameterizationQuantity("param vert local test", vertParamLocal);
  }


  { // Add a curve network from the edges
    std::vector<std::array<size_t, 2>> edges;
    for (size_t iF = 0; iF < nFaces; iF++) {
      std::vector<size_t>& face = faceIndices[iF];

      for (size_t iV = 0; iV < face.size(); iV++) {
        size_t i0 = face[iV];
        size_t i1 = face[(iV + 1) % face.size()];
        if (i0 < i1) {
          edges.push_back({i0, i1});
        }
      }
    }

    std::string curveName = niceName + " curves";
    constructDemoCurveNetwork(curveName, vertexPositionsGLM, edges);
  }
}

void addVolumeGrid() {

  // uint32_t dimX = 4;
  // uint32_t dimY = 5;
  // uint32_t dimZ = 6;
  // glm::vec3 bound_low{-1., -2., -5.};
  // glm::vec3 bound_high{3., -1., 1.};

  // uint32_t dimX = 256;
  // uint32_t dimY = 256;
  // uint32_t dimZ = 256;
  // uint32_t dimX = 128;
  // uint32_t dimY = 128;
  // uint32_t dimZ = 128;
  // uint32_t dimX = 2;
  // uint32_t dimY = 2;
  // uint32_t dimZ = 2;
  uint32_t dimX = 20;
  uint32_t dimY = 20;
  uint32_t dimZ = 20;
  // glm::vec3 bound_low{0., 0., 0.};
  // glm::vec3 bound_high{1., 1., 1.};
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};


  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);
  polyscope::registerPointCloud("corners", std::vector<glm::vec3>{bound_low, bound_high});

  psGrid->setEdgeWidth(1.0);

  // Scalar quantities
  auto torusSDF = [](glm::vec3 p) {
    float scale = 0.5;
    p /= scale;
    p += glm::vec3{1., 0., 1.};
    glm::vec2 t{1., 0.3};
    glm::vec2 pxz{p.x, p.z};
    glm::vec2 q = glm::vec2(glm::length(pxz) - t.x, p.y);
    return (glm::length(q) - t.y) * scale;
  };


  polyscope::VolumeGridNodeScalarQuantity* qNode =
      psGrid->addNodeScalarQuantityFromCallable("torus sdf node", torusSDF);
  qNode->setEnabled(true);

  polyscope::VolumeGridCellScalarQuantity* qCell =
      psGrid->addCellScalarQuantityFromCallable("torus sdf cell", torusSDF);
  qCell->setEnabled(true);

  // use this to check ordering thing
  auto xCoord = [](glm::vec3 p) { return p.x; };
  auto yCoord = [](glm::vec3 p) { return p.y; };
  auto zCoord = [](glm::vec3 p) { return p.z; };

  polyscope::VolumeGridNodeScalarQuantity* qNode_X = psGrid->addNodeScalarQuantityFromCallable("node X", xCoord);
  polyscope::VolumeGridNodeScalarQuantity* qNode_Y = psGrid->addNodeScalarQuantityFromCallable("node Y", yCoord);
  polyscope::VolumeGridNodeScalarQuantity* qNode_Z = psGrid->addNodeScalarQuantityFromCallable("node Z", zCoord);

  polyscope::VolumeGridCellScalarQuantity* qCell_X = psGrid->addCellScalarQuantityFromCallable("cell X", xCoord);
  polyscope::VolumeGridCellScalarQuantity* qCell_Y = psGrid->addCellScalarQuantityFromCallable("cell Y", yCoord);
  polyscope::VolumeGridCellScalarQuantity* qCell_Z = psGrid->addCellScalarQuantityFromCallable("cell Z", zCoord);
}


void addSparseVolumeGrid() {
  glm::vec3 origin{-1., -1., -1.};
  int32_t N = 20;
  glm::vec3 cellWidth{1.0f / N, 1.0f / N, 1.0f / N};

  // Build a spherical shell of occupied cells
  std::vector<glm::ivec3> occupiedCells;
  for (int i = -N; i <= N; i++) {
    for (int j = -N; j <= N; j++) {
      for (int k = -N; k <= N; k++) {
        glm::vec3 cellCenter = origin + (glm::vec3(i, j, k) + 0.5f) * cellWidth;
        float dist = glm::length(cellCenter - origin);
        if (dist >= 0.35f && dist <= 1.0f) {
          occupiedCells.push_back({i, j, k});
        }
      }
    }
  }
  // Gather all unique nodes for the occupied cells
  // Node (ci+dx, cj+dy, ck+dz) for dx,dy,dz in {0,1}
  std::set<std::tuple<int, int, int>> nodeSet;
  for (const auto& ci : occupiedCells) {
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          nodeSet.insert({ci.x + dx, ci.y + dy, ci.z + dz});
        }
      }
    }
  }

  std::cout << "adding sparse volume grid with " << occupiedCells.size() << " cells" << std::endl;
  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("sparse grid", origin, cellWidth, occupiedCells);

  // Cell scalar: distance from origin
  {
    std::vector<float> cellDist(occupiedCells.size());
    for (size_t i = 0; i < occupiedCells.size(); i++) {
      glm::vec3 cellCenter = origin + (glm::vec3(occupiedCells[i]) + 0.5f) * cellWidth;
      cellDist[i] = glm::length(cellCenter - origin);
    }
    psGrid->addCellScalarQuantity("cell distance", cellDist);
  }

  // Node scalar: x-coordinate at node positions
  {

    std::vector<glm::ivec3> nodeIndices;
    std::vector<float> nodeValues;
    for (const auto& nodeInds : nodeSet) {
      int32_t ni, nj, nk;
      std::tie(ni, nj, nk) = nodeInds;
      nodeIndices.push_back({ni, nj, nk});
      glm::vec3 nodePos = origin + glm::vec3(ni, nj, nk) * cellWidth;
      nodeValues.push_back(nodePos.x);
    }
    psGrid->addNodeScalarQuantity("node x-coord", nodeIndices, nodeValues);
  }

  // Cell color: random colors
  {
    std::vector<glm::vec3> cellColors(occupiedCells.size());
    for (size_t i = 0; i < occupiedCells.size(); i++) {
      cellColors[i] = glm::vec3{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()};
    }
    psGrid->addCellColorQuantity("cell color", cellColors);
  }

  // Node color: random colors
  {
    std::vector<glm::ivec3> nodeIndices;
    std::vector<glm::vec3> nodeColors;
    for (const auto& nodeInds : nodeSet) {
      int32_t ni, nj, nk;
      std::tie(ni, nj, nk) = nodeInds;
      nodeIndices.push_back({ni, nj, nk});
      nodeColors.push_back(glm::vec3{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()});
    }
    psGrid->addNodeColorQuantity("node color", nodeIndices, nodeColors);
  }
}


void loadFloatingImageData(polyscope::CameraView* targetView = nullptr) {

  // load an image from disk as example data
  std::string imagePath = "test_image.png";
  // std::string imagePath = "test_image_transparent.png";

  int width, height, nComp;
  unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &nComp, 4);
  if (!data) {
    polyscope::warning("failed to load image from " + imagePath);
    return;
  }
  bool hasAlpha = (nComp == 4);

  // Parse the data in to a float array
  std::vector<std::array<float, 3>> imageColor(width * height);
  std::vector<std::array<float, 4>> imageColorAlpha(width * height);
  std::vector<float> imageScalar(width * height);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      int pixInd = (j * width + i) * nComp;
      unsigned char pR = data[pixInd + 0];
      unsigned char pG = data[pixInd + 1];
      unsigned char pB = data[pixInd + 2];
      unsigned char pA = 255;
      if (nComp == 4) pA = data[pixInd + 3];

      // color
      std::array<float, 3> val{pR / 255.f, pG / 255.f, pB / 255.f};
      imageColor[j * width + i] = val;

      // scalar
      imageScalar[j * width + i] = (val[0] + val[1] + val[2]) / 3.;

      // color alpha
      std::array<float, 4> valA{pR / 255.f, pG / 255.f, pB / 255.f, pA / 255.f};
      imageColorAlpha[j * width + i] = valA;
    }
  }

  if (targetView == nullptr) {
    polyscope::addColorImageQuantity("test color image", width, height, imageColor, polyscope::ImageOrigin::UpperLeft);
    polyscope::addScalarImageQuantity("test scalar image", width, height, imageScalar,
                                      polyscope::ImageOrigin::UpperLeft);

    if (hasAlpha) {
      polyscope::addColorAlphaImageQuantity("test color alpha image", width, height, imageColorAlpha,
                                            polyscope::ImageOrigin::UpperLeft);
    }
  } else {
    targetView->addColorImageQuantity("test color image", width, height, imageColor, polyscope::ImageOrigin::UpperLeft);
    targetView->addScalarImageQuantity("test scalar image", width, height, imageScalar,
                                       polyscope::ImageOrigin::UpperLeft);

    if (hasAlpha) {
      targetView->addColorAlphaImageQuantity("test color alpha image", width, height, imageColorAlpha,
                                             polyscope::ImageOrigin::UpperLeft);
    }
  }
}

void addImplicitRendersFromCurrentView() {

  // sample sdf
  auto torusSDF = [](glm::vec3 p) {
    float scale = 0.5;
    p /= scale;
    p += glm::vec3{1., 0., 1.};
    glm::vec2 t{1., 0.3};
    glm::vec2 pxz{p.x, p.z};
    glm::vec2 q = glm::vec2(glm::length(pxz) - t.x, p.y);
    return (glm::length(q) - t.y) * scale;
  };
  auto boxFrameSDF = [](glm::vec3 p) {
    float scale = 0.5;
    p /= scale;
    float b = 1.;
    float e = 0.1;
    p = glm::abs(p) - b;
    glm::vec3 q = glm::abs(p + e) - e;

    float out = glm::min(
        glm::min(
            glm::length(glm::max(glm::vec3(p.x, q.y, q.z), 0.0f)) + glm::min(glm::max(p.x, glm::max(q.y, q.z)), 0.0f),
            glm::length(glm::max(glm::vec3(q.x, p.y, q.z), 0.0f)) + glm::min(glm::max(q.x, glm::max(p.y, q.z)), 0.0f)),
        glm::length(glm::max(glm::vec3(q.x, q.y, p.z), 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, p.z)), 0.0f));
    return out * scale;
  };

  auto colorFunc = [](glm::vec3 p) {
    glm::vec3 color{0., 0., 0.};
    if (p.x > 0) {
      color += glm::vec3{1.0, 0.0, 0.0};
    }
    if (p.y > 0) {
      color += glm::vec3{0.0, 1.0, 0.0};
    }
    if (p.z > 0) {
      color += glm::vec3{0.0, 0.0, 1.0};
    }
    return color;
  };

  auto scalarFunc = [](glm::vec3 p) { return p.x; };

  polyscope::ImplicitRenderOpts opts;
  // opts.mode = polyscope::ImplicitRenderMode::FixedStep;
  polyscope::ImplicitRenderMode mode = polyscope::ImplicitRenderMode::SphereMarch;
  // polyscope::ImplicitRenderMode mode = polyscope::ImplicitRenderMode::FixedStep;
  opts.subsampleFactor = 2;

  polyscope::DepthRenderImageQuantity* img = polyscope::renderImplicitSurface("torus sdf", torusSDF, mode, opts);
  polyscope::DepthRenderImageQuantity* img2 = polyscope::renderImplicitSurface("box sdf", boxFrameSDF, mode, opts);
  polyscope::ColorRenderImageQuantity* img2Color =
      polyscope::renderImplicitSurfaceColor("box sdf color", boxFrameSDF, colorFunc, mode, opts);
  polyscope::RawColorRenderImageQuantity* img2rawColor =
      polyscope::renderImplicitSurfaceRawColor("box sdf raw color", boxFrameSDF, colorFunc, mode, opts);
  polyscope::ScalarRenderImageQuantity* imgScalar =
      polyscope::renderImplicitSurfaceScalar("torus sdf scalar", torusSDF, scalarFunc, mode, opts);
}

void dropCameraView() {
  polyscope::CameraView* cam1 =
      polyscope::registerCameraView("dropped cam", polyscope::view::getCameraParametersForCurrentView());

  loadFloatingImageData(cam1);
}

void processFileDotMesh(std::string filename) {
  std::vector<std::array<double, 3>> verts;
  std::vector<std::array<int64_t, 8>> cells;
  parseVolumeDotMesh(filename, verts, cells);
  std::string niceName = polyscope::guessNiceNameFromPath(filename);

  std::cout << "parsed mesh with " << verts.size() << " verts and " << cells.size() << " cells\n";

  auto ps_vol = polyscope::registerVolumeMesh(niceName, verts, cells);

  // Add some scalar quantities
  std::vector<std::array<double, 3>> randColorV(verts.size());
  std::vector<std::array<double, 3>> randVecV(verts.size());
  std::vector<double> scalarV(verts.size());
  std::vector<std::array<double, 3>> randColorC(cells.size());
  std::vector<std::array<double, 3>> randVecC(cells.size());
  std::vector<double> scalarC(cells.size());
  for (size_t i = 0; i < verts.size(); i++) {
    randColorV[i] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
    randVecV[i] = {{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5}};
    scalarV[i] = verts[i][0];
  }
  for (size_t i = 0; i < cells.size(); i++) {
    randColorC[i] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
    randVecC[i] = {{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5}};
    scalarC[i] = polyscope::randomUnit();
  }

  polyscope::getVolumeMesh(niceName)->addVertexColorQuantity("random color", randColorV);
  polyscope::getVolumeMesh(niceName)->addCellColorQuantity("random color2", randColorC);
  polyscope::getVolumeMesh(niceName)->addVertexScalarQuantity("scalar Q", scalarV);
  polyscope::getVolumeMesh(niceName)->addCellScalarQuantity("scalar Q2", scalarC);
  polyscope::getVolumeMesh(niceName)->addVertexVectorQuantity("random vec", randVecV);
  polyscope::getVolumeMesh(niceName)->addCellVectorQuantity("random vec2", randVecC);
}

void addDataToPointCloud(std::string pointCloudName, const std::vector<glm::vec3>& points) {


  // Add some scalar quantities
  std::vector<double> xC(points.size());
  std::vector<std::array<double, 3>> randColor(points.size());
  std::vector<double> cat(points.size());
  for (size_t i = 0; i < points.size(); i++) {
    xC[i] = points[i].x;
    randColor[i] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
    cat[i] = i * 12 / points.size();
  }
  polyscope::getPointCloud(pointCloudName)->addScalarQuantity("xC", xC);
  polyscope::getPointCloud(pointCloudName)->addColorQuantity("random color", randColor);
  polyscope::getPointCloud(pointCloudName)->addColorQuantity("random color2", randColor);
  polyscope::getPointCloud(pointCloudName)->addScalarQuantity("categorical", cat, polyscope::DataType::CATEGORICAL);


  // Add some vector quantities
  std::vector<glm::vec3> randVec(points.size());
  std::vector<glm::vec3> centerNormalVec(points.size());
  std::vector<glm::vec3> toZeroVec(points.size());
  for (size_t i = 0; i < points.size(); i++) {
    randVec[i] = (float)(10. * polyscope::randomUnit()) *
                 glm::vec3{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()};
    centerNormalVec[i] = glm::normalize(points[i]);
    toZeroVec[i] = -points[i];
  }
  polyscope::getPointCloud(pointCloudName)->addVectorQuantity("random vector", randVec);
  polyscope::getPointCloud(pointCloudName)->addVectorQuantity("unit 'normal' vector", centerNormalVec);
  polyscope::getPointCloud(pointCloudName)->addVectorQuantity("to zero", toZeroVec, polyscope::VectorType::AMBIENT);

  // loadFloatingImageData(polyscope::getPointCloud(pointCloudName));
}

// PLY files get loaded as point clouds
void processFilePLY(std::string filename) {

  // load the data
  happly::PLYData plyIn(filename);
  std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();

  polyscope::PointCloud* psCloud = polyscope::registerPointCloud(polyscope::guessNiceNameFromPath(filename), vPos);
  // psCloud->setPointRenderMode(polyscope::PointRenderMode::Square);

  // Try to add colors if we have them
  try {
    std::vector<std::array<unsigned char, 3>> vColor = plyIn.getVertexColors();
    std::vector<std::array<float, 3>> vColorF(vColor.size());
    for (size_t i = 0; i < vColorF.size(); i++) {
      for (int j = 0; j < 3; j++) {
        vColorF[i][j] = vColor[i][j] / 255.;
      }
    }
    psCloud->addColorQuantity("color", vColorF)->setEnabled(true);
  } catch (const std::exception& e) {
  }
}


void processFile(std::string filename) {
  // Dispatch to correct varient
  if (endsWith(filename, ".obj")) {
    processFileOBJ(filename);
  } else if (endsWith(filename, ".mesh")) {
    processFileDotMesh(filename);
  } else if (endsWith(filename, ".ply")) {
    // PLY files get loaded as point clouds
    processFilePLY(filename);
  } else {
    std::cerr << "Unrecognized file type for " << filename << std::endl;
  }
}


// ============================================================
// ============== Multi-Viewport Visual Test Scenes ===========
// ============================================================

namespace {

void makeSphereMesh(std::vector<glm::vec3>& verts, std::vector<std::array<size_t, 3>>& faces, glm::vec3 center,
                    float radius, int nu = 32, int nv = 20) {
  verts.clear();
  faces.clear();
  const float pi = 3.14159265358979323846f;
  for (int i = 0; i < nv; i++) {
    float v = 0.05f * pi + 0.9f * pi * static_cast<float>(i) / (nv - 1);
    for (int j = 0; j < nu; j++) {
      float u = 2.0f * pi * static_cast<float>(j) / nu;
      verts.push_back(center + radius * glm::vec3{std::sin(v) * std::cos(u), std::cos(v), std::sin(v) * std::sin(u)});
    }
  }
  for (int i = 0; i < nv - 1; i++) {
    for (int j = 0; j < nu; j++) {
      size_t j2 = (j + 1) % nu;
      size_t v00 = i * nu + j;
      size_t v01 = i * nu + j2;
      size_t v10 = (i + 1) * nu + j;
      size_t v11 = (i + 1) * nu + j2;
      faces.push_back({v00, v10, v11});
      faces.push_back({v00, v11, v01});
    }
  }
}

void makeTorusMesh(std::vector<glm::vec3>& verts, std::vector<std::array<size_t, 3>>& faces, glm::vec3 center,
                   float majorR, float minorR, int nu = 40, int nv = 24) {
  verts.clear();
  faces.clear();
  const float pi = 3.14159265358979323846f;
  for (int i = 0; i < nu; i++) {
    float u = 2.0f * pi * static_cast<float>(i) / nu;
    float cu = std::cos(u), su = std::sin(u);
    for (int j = 0; j < nv; j++) {
      float v = 2.0f * pi * static_cast<float>(j) / nv;
      float r = majorR + minorR * std::cos(v);
      verts.push_back(center + glm::vec3{r * cu, minorR * std::sin(v), r * su});
    }
  }
  for (int i = 0; i < nu; i++) {
    size_t i2 = (i + 1) % nu;
    for (int j = 0; j < nv; j++) {
      size_t j2 = (j + 1) % nv;
      size_t v00 = i * nv + j;
      size_t v01 = i * nv + j2;
      size_t v10 = i2 * nv + j;
      size_t v11 = i2 * nv + j2;
      faces.push_back({v00, v10, v11});
      faces.push_back({v00, v11, v01});
    }
  }
}

void makeWireframeBox(std::vector<glm::vec3>& nodes, std::vector<std::array<size_t, 2>>& edges, glm::vec3 center,
                      glm::vec3 halfExtents) {
  nodes = {
      center + glm::vec3{-halfExtents.x, -halfExtents.y, -halfExtents.z},
      center + glm::vec3{+halfExtents.x, -halfExtents.y, -halfExtents.z},
      center + glm::vec3{+halfExtents.x, +halfExtents.y, -halfExtents.z},
      center + glm::vec3{-halfExtents.x, +halfExtents.y, -halfExtents.z},
      center + glm::vec3{-halfExtents.x, -halfExtents.y, +halfExtents.z},
      center + glm::vec3{+halfExtents.x, -halfExtents.y, +halfExtents.z},
      center + glm::vec3{+halfExtents.x, +halfExtents.y, +halfExtents.z},
      center + glm::vec3{-halfExtents.x, +halfExtents.y, +halfExtents.z},
  };
  edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
}

polyscope::PointCloud* registerDemoCloud(const std::string& name, glm::vec3 center, float radius, size_t nPts = 600) {
  std::vector<glm::vec3> pts;
  std::vector<glm::vec3> cols;
  std::vector<glm::vec3> vecs;
  pts.reserve(nPts);
  cols.reserve(nPts);
  vecs.reserve(nPts);
  for (size_t i = 0; i < nPts; i++) {
    float t = static_cast<float>(i) / nPts;
    float a = 18.0f * t;
    float y = (t - 0.5f) * 2.0f * radius;
    float r = std::sqrt(std::max(0.0f, radius * radius - y * y));
    glm::vec3 dir{r * std::cos(a), y, r * std::sin(a)};
    pts.push_back(center + dir);
    cols.push_back({0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a * 0.7f), 1.0f - t});
    vecs.push_back(0.25f * glm::normalize(dir + glm::vec3{1e-4f, 0.f, 0.f}));
  }
  auto* pc = polyscope::registerPointCloud(name, pts);
  pc->addColorQuantity("helix color", cols)->setEnabled(true);
  pc->addVectorQuantity("outward vec", vecs);
  return pc;
}

static bool demoShowViewportBorders = true;
static bool demoSlicePlaneEnabled = false;
static std::string activeDemoSceneName = "";

void syncDemoSlicePlane() {
  polyscope::removeAllSlicePlanes();
  if (demoSlicePlaneEnabled) {
    polyscope::SlicePlane* sp = polyscope::addSceneSlicePlane();
    sp->setPose(glm::vec3{0.0f, 0.45f, 0.0f}, glm::vec3{0.8f, 0.3f, 0.5f});
    sp->setDrawPlane(true);
    sp->setDrawWidget(true);
  }
}

void resetDemoScene() {
  polyscope::removeAllStructures();
  polyscope::removeAllSlicePlanes();
  demoSlicePlaneEnabled = false;
  polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Automatic;
  polyscope::view::setViewportsShareNavigation(false);
}

// Scene 1: 2x2 non-overlapping grid mixing transparent meshes, opaque cores, curve networks, point clouds, and ground
// reflections/shadows from 4 distinct cameras.
void loadViewportScene_2x2TransparentGrid() {
  resetDemoScene();
  activeDemoSceneName = "2x2 Transparent Grid";

  polyscope::view::createViewportGrid(2, 2);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;

  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;
  std::vector<glm::vec3> CN;
  std::vector<std::array<size_t, 2>> CE;

  // VP 0 & 3: translucent outer sphere + opaque inner core + wireframe cage
  makeSphereMesh(V, F, {0.f, 0.65f, 0.f}, 0.58f);
  auto* outerSphere = polyscope::registerSurfaceMesh("outer glass sphere", V, F);
  outerSphere->setSurfaceColor({0.25f, 0.65f, 0.95f});
  outerSphere->setTransparency(0.45);
  outerSphere->setViewportVisibilityAll(false);
  outerSphere->setViewportVisibility(0, true);
  outerSphere->setViewportVisibility(3, true);

  makeSphereMesh(V, F, {0.f, 0.65f, 0.f}, 0.28f);
  auto* innerCore = polyscope::registerSurfaceMesh("opaque inner core", V, F);
  innerCore->setSurfaceColor({0.95f, 0.35f, 0.15f});
  innerCore->setViewportVisibilityAll(false);
  innerCore->setViewportVisibility(0, true);
  innerCore->setViewportVisibility(3, true);

  makeWireframeBox(CN, CE, {0.f, 0.65f, 0.f}, {0.68f, 0.68f, 0.68f});
  auto* cage = polyscope::registerCurveNetwork("wireframe cage", CN, CE);
  cage->setColor({0.18f, 0.18f, 0.22f});
  cage->setRadius(0.015);
  cage->setViewportVisibilityAll(false);
  cage->setViewportVisibility(0, true);
  cage->setViewportVisibility(3, true);

  // VP 1 & 3: translucent torus + colorful helix point cloud
  makeTorusMesh(V, F, {0.f, 0.55f, 0.f}, 0.52f, 0.19f);
  auto* torus = polyscope::registerSurfaceMesh("emerald torus", V, F);
  torus->setSurfaceColor({0.15f, 0.80f, 0.45f});
  torus->setTransparency(0.55);
  torus->setViewportVisibilityAll(false);
  torus->setViewportVisibility(1, true);
  torus->setViewportVisibility(3, true);

  auto* cloud = registerDemoCloud("helix cloud", {0.f, 0.65f, 0.f}, 0.72f, 700);
  cloud->setPointRadius(0.014);
  cloud->setTransparency(0.75);
  cloud->setViewportVisibilityAll(false);
  cloud->setViewportVisibility(1, true);
  cloud->setViewportVisibility(3, true);

  // VP 2: 3 concentric nested translucent shells (heavy depth peeling test)
  std::array<std::tuple<float, glm::vec3, float>, 3> shells = {
      std::make_tuple(0.65f, glm::vec3{0.92f, 0.22f, 0.45f}, 0.32f),
      std::make_tuple(0.45f, glm::vec3{0.95f, 0.75f, 0.15f}, 0.50f),
      std::make_tuple(0.24f, glm::vec3{0.20f, 0.30f, 0.90f}, 1.00f),
  };
  for (int s = 0; s < 3; s++) {
    makeSphereMesh(V, F, {0.f, 0.68f, 0.f}, std::get<0>(shells[s]));
    auto* sm = polyscope::registerSurfaceMesh("nested shell " + std::to_string(s), V, F);
    sm->setSurfaceColor(std::get<1>(shells[s]));
    sm->setTransparency(std::get<2>(shells[s]));
    sm->setViewportVisibilityExclusive(2);
  }

  // Distinct camera angles per viewport
  std::array<glm::vec3, 4> camPos = {
      glm::vec3{1.9f, 1.4f, 1.9f},
      glm::vec3{-2.0f, 1.6f, 1.7f},
      glm::vec3{0.0f, 1.2f, 2.5f},
      glm::vec3{2.4f, 0.55f, 1.6f}, // low grazing angle for prominent ground reflections
  };
  for (int i = 0; i < 4; i++) {
    polyscope::view::activateViewport(i);
    polyscope::view::lookAt(camPos[i], glm::vec3{0.f, 0.55f, 0.f});
  }
  polyscope::view::activateViewport(0);
}

// Scene 2: Overlapping insets (3 layers: full-window VP 0, bottom-right inset VP 1, and overlapping VP 2 on top of
// both VP 0 and VP 1). Exercises scissor-clearing of both sceneBuffer (Normal) and sceneBufferFinal (Pretty), plus
// non-zero viewport origins for ground plane reflections & shadows.
void loadViewportScene_OverlappingInsets() {
  resetDemoScene();
  activeDemoSceneName = "Overlapping Insets (3 Layers)";

  polyscope::view::setNumViewports(3);
  // VP 0: full window
  polyscope::view::activateViewport(0);
  polyscope::view::setActiveViewportRelativeLocation(0.0f, 0.0f);
  polyscope::view::setActiveViewportRelativeSize(1.0f, 1.0f);
  // VP 1: lower-right inset
  polyscope::view::activateViewport(1);
  polyscope::view::setActiveViewportRelativeLocation(0.64f, 0.56f);
  polyscope::view::setActiveViewportRelativeSize(0.34f, 0.40f);
  // VP 2: smaller inset overlapping BOTH VP 0 and VP 1's upper-left corner
  polyscope::view::activateViewport(2);
  polyscope::view::setActiveViewportRelativeLocation(0.46f, 0.36f);
  polyscope::view::setActiveViewportRelativeSize(0.28f, 0.32f);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;

  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;
  std::vector<glm::vec3> CN;
  std::vector<std::array<size_t, 2>> CE;

  // VP 0 (background full window): big warm coral torus + opaque gold sphere
  makeTorusMesh(V, F, {0.f, 0.65f, 0.f}, 0.72f, 0.24f);
  auto* mainTorus = polyscope::registerSurfaceMesh("main coral torus", V, F);
  mainTorus->setSurfaceColor({0.92f, 0.28f, 0.22f});
  mainTorus->setTransparency(0.60);
  mainTorus->setViewportVisibilityExclusive(0);

  makeSphereMesh(V, F, {0.f, 0.65f, 0.f}, 0.35f);
  auto* mainSphere = polyscope::registerSurfaceMesh("main gold core", V, F);
  mainSphere->setSurfaceColor({0.95f, 0.75f, 0.20f});
  mainSphere->setViewportVisibilityExclusive(0);

  // VP 1 (inset 1): translucent emerald sphere + point cloud
  makeSphereMesh(V, F, {0.f, 0.65f, 0.f}, 0.55f);
  auto* inset1Sphere = polyscope::registerSurfaceMesh("inset1 emerald shell", V, F);
  inset1Sphere->setSurfaceColor({0.12f, 0.82f, 0.42f});
  inset1Sphere->setTransparency(0.50);
  inset1Sphere->setViewportVisibilityExclusive(1);

  auto* inset1Cloud = registerDemoCloud("inset1 cloud", {0.f, 0.65f, 0.f}, 0.35f, 500);
  inset1Cloud->setPointRadius(0.018);
  inset1Cloud->setViewportVisibilityExclusive(1);

  // VP 2 (inset 2 overlapping VP 0 and VP 1): cobalt torus + wireframe cage
  makeTorusMesh(V, F, {0.f, 0.65f, 0.f}, 0.50f, 0.20f);
  auto* inset2Torus = polyscope::registerSurfaceMesh("inset2 cobalt torus", V, F);
  inset2Torus->setSurfaceColor({0.18f, 0.42f, 0.95f});
  inset2Torus->setTransparency(0.55);
  inset2Torus->setViewportVisibilityExclusive(2);

  makeWireframeBox(CN, CE, {0.f, 0.65f, 0.f}, {0.62f, 0.45f, 0.62f});
  auto* inset2Cage = polyscope::registerCurveNetwork("inset2 cage", CN, CE);
  inset2Cage->setColor({0.95f, 0.60f, 0.10f});
  inset2Cage->setRadius(0.018);
  inset2Cage->setViewportVisibilityExclusive(2);

  polyscope::view::activateViewport(0);
  polyscope::view::lookAt(glm::vec3{1.7f, 1.1f, 2.3f}, glm::vec3{0.f, 0.55f, 0.f});
  polyscope::view::activateViewport(1);
  polyscope::view::lookAt(glm::vec3{-1.8f, 1.2f, 1.8f}, glm::vec3{0.f, 0.55f, 0.f});
  polyscope::view::activateViewport(2);
  polyscope::view::lookAt(glm::vec3{0.4f, 1.8f, 1.9f}, glm::vec3{0.f, 0.55f, 0.f});
  polyscope::view::activateViewport(0);
}

// Scene 3: 3x4 (12-viewport) grid stress test with per-viewport colored transparent meshes + shared point cloud.
void loadViewportScene_3x4StressGrid() {
  resetDemoScene();
  activeDemoSceneName = "3x4 Grid (12 Viewports)";

  polyscope::view::createViewportGrid(4, 3);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;

  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;

  const float pi = 3.14159265358979323846f;
  for (int i = 0; i < 12; i++) {
    float hue = static_cast<float>(i) / 12.0f;
    glm::vec3 col{0.5f + 0.45f * std::cos(2.0f * pi * hue),
                  0.5f + 0.45f * std::cos(2.0f * pi * (hue + 0.333f)),
                  0.5f + 0.45f * std::cos(2.0f * pi * (hue + 0.666f))};
    if (i % 2 == 0) {
      makeTorusMesh(V, F, {0.f, 0.55f, 0.f}, 0.50f, 0.18f, 32, 18);
    } else {
      makeSphereMesh(V, F, {0.f, 0.58f, 0.f}, 0.50f, 24, 16);
    }
    auto* m = polyscope::registerSurfaceMesh("vp_" + std::to_string(i) + "_shape", V, F);
    m->setSurfaceColor(col);
    m->setTransparency(0.55);
    m->setViewportVisibilityExclusive(i);

    polyscope::view::activateViewport(i);
    float ang = 2.0f * pi * static_cast<float>(i) / 12.0f;
    polyscope::view::lookAt(glm::vec3{2.1f * std::cos(ang), 0.9f + 0.4f * (i % 3), 2.1f * std::sin(ang)},
                            glm::vec3{0.f, 0.5f, 0.f});
  }

  // Shared opaque inner sphere + translucent point cloud visible in all 12 viewports
  makeSphereMesh(V, F, {0.f, 0.55f, 0.f}, 0.22f, 20, 14);
  auto* core = polyscope::registerSurfaceMesh("shared core", V, F);
  core->setSurfaceColor({0.95f, 0.95f, 0.95f});

  auto* pc = registerDemoCloud("shared cloud", {0.f, 0.58f, 0.f}, 0.72f, 500);
  pc->setPointRadius(0.014);
  pc->setTransparency(0.7);

  polyscope::view::activateViewport(0);
}

// Scene 4: 2x2 ShadowOnly isolation test under Pretty transparency (VP 1 intentionally empty so any shadow leak is
// immediately obvious; VP 0/2/3 cast distinct silhouettes from different camera angles).
void loadViewportScene_ShadowOnlyIsolation() {
  resetDemoScene();
  activeDemoSceneName = "2x2 ShadowOnly Isolation";

  polyscope::view::createViewportGrid(2, 2);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;
  polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Manual;
  polyscope::options::groundPlaneHeight = 0.0f;

  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;
  std::vector<glm::vec3> CN;
  std::vector<std::array<size_t, 2>> CE;

  // VP 0: floating translucent red torus
  makeTorusMesh(V, F, {0.f, 0.75f, 0.f}, 0.55f, 0.18f);
  auto* m0 = polyscope::registerSurfaceMesh("vp0 torus caster", V, F);
  m0->setSurfaceColor({0.90f, 0.25f, 0.20f});
  m0->setTransparency(0.6);
  m0->setViewportVisibilityExclusive(0);

  // VP 1: intentionally empty (should show pure clean background, zero shadow!)

  // VP 2: floating twin blue spheres
  makeSphereMesh(V, F, {-0.38f, 0.65f, 0.f}, 0.30f);
  auto* m2a = polyscope::registerSurfaceMesh("vp2 sphere A", V, F);
  m2a->setSurfaceColor({0.20f, 0.45f, 0.95f});
  m2a->setTransparency(0.55);
  m2a->setViewportVisibilityExclusive(2);

  makeSphereMesh(V, F, {0.38f, 0.95f, 0.f}, 0.25f);
  auto* m2b = polyscope::registerSurfaceMesh("vp2 sphere B", V, F);
  m2b->setSurfaceColor({0.20f, 0.80f, 0.85f});
  m2b->setViewportVisibilityExclusive(2);

  // VP 3: tall curve-network tower
  makeWireframeBox(CN, CE, {0.f, 0.80f, 0.f}, {0.35f, 0.65f, 0.35f});
  auto* c3 = polyscope::registerCurveNetwork("vp3 tower", CN, CE);
  c3->setColor({0.25f, 0.72f, 0.35f});
  c3->setRadius(0.03);
  c3->setViewportVisibilityExclusive(3);

  std::array<glm::vec3, 4> camPos = {
      glm::vec3{0.0f, 2.2f, 2.5f},
      glm::vec3{0.0f, 2.2f, 2.5f},
      glm::vec3{1.8f, 2.0f, 1.8f},
      glm::vec3{-2.0f, 1.8f, 2.2f},
  };
  for (int i = 0; i < 4; i++) {
    polyscope::view::activateViewport(i);
    polyscope::view::lookAt(camPos[i], glm::vec3{0.f, 0.45f, 0.f});
  }
  polyscope::view::activateViewport(0);
}

// Scene 5: Single viewport for quick comparison
void loadViewportScene_Single() {
  resetDemoScene();
  activeDemoSceneName = "Single Viewport";

  polyscope::view::createViewportGrid(1, 1);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;

  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;
  makeTorusMesh(V, F, {0.f, 0.65f, 0.f}, 0.60f, 0.20f);
  auto* torus = polyscope::registerSurfaceMesh("demo torus", V, F);
  torus->setSurfaceColor({0.25f, 0.65f, 0.95f});
  torus->setTransparency(0.5);

  makeSphereMesh(V, F, {0.f, 0.65f, 0.f}, 0.28f);
  auto* core = polyscope::registerSurfaceMesh("demo core", V, F);
  core->setSurfaceColor({0.95f, 0.35f, 0.15f});

  auto* pc = registerDemoCloud("demo cloud", {0.f, 0.65f, 0.f}, 0.85f, 600);
  pc->setPointRadius(0.015);
  pc->setTransparency(0.75);

  polyscope::view::lookAt(glm::vec3{2.0f, 1.3f, 2.0f}, glm::vec3{0.f, 0.55f, 0.f});
}

void drawViewportBorderOverlay() {
  if (!demoShowViewportBorders) return;
  ImDrawList* drawList = ImGui::GetBackgroundDrawList();
  int nVP = polyscope::view::getNumViewports();
  int activeVP = polyscope::view::getActiveViewportIndex();
  for (int i = 0; i < nVP; i++) {
    glm::vec4 r = polyscope::view::getViewportImGuiRect(i);
    if (r.z <= 0.f || r.w <= 0.f) continue;
    ImVec2 pMin(r.x + 0.5f, r.y + 0.5f);
    ImVec2 pMax(r.x + r.z - 0.5f, r.y + r.w - 0.5f);
    ImU32 borderCol = (i == activeVP) ? IM_COL32(255, 165, 0, 220) : IM_COL32(80, 80, 80, 170);
    float thickness = (i == activeVP) ? 2.5f : 1.5f;
    drawList->AddRect(pMin, pMax, borderCol, 0.0f, 0, thickness);

    std::string label = "VP " + std::to_string(i);
    ImVec2 textPos(r.x + 8.0f, r.y + r.w - 22.0f);
    drawList->AddRectFilled(ImVec2(textPos.x - 4.0f, textPos.y - 2.0f), ImVec2(textPos.x + 42.0f, textPos.y + 16.0f),
                            IM_COL32(20, 20, 20, 160), 3.0f);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 230), label.c_str());
  }
}

} // namespace

void callback() {
  static int numPoints = 2000;
  static float param = 3.14;
  static int loadedMat = 1;
  static bool depthClick = false;

  drawViewportBorderOverlay();

  ImGui::PushItemWidth(100 * polyscope::options::uiScale);

  // Quick multi-viewport visual verification controls
  ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Viewport Render Demo")) {
    ImGui::Text("Scene: %s", activeDemoSceneName.empty() ? "(custom)" : activeDemoSceneName.c_str());
    ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / std::max(1e-3f, ImGui::GetIO().Framerate));

    if (ImGui::Button("2x2 Transparent Grid")) loadViewportScene_2x2TransparentGrid();
    if (ImGui::Button("Overlapping Insets (3 Layers)")) loadViewportScene_OverlappingInsets();
    if (ImGui::Button("3x4 Grid (12 Viewports)")) loadViewportScene_3x4StressGrid();
    if (ImGui::Button("2x2 ShadowOnly Isolation")) loadViewportScene_ShadowOnlyIsolation();
    if (ImGui::Button("Single Viewport")) loadViewportScene_Single();

    ImGui::Separator();
    ImGui::TextUnformatted("Transparency:");
    if (ImGui::RadioButton("None##t", polyscope::options::transparencyMode == polyscope::TransparencyMode::None))
      polyscope::options::transparencyMode = polyscope::TransparencyMode::None;
    ImGui::SameLine();
    if (ImGui::RadioButton("Simple##t", polyscope::options::transparencyMode == polyscope::TransparencyMode::Simple))
      polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
    ImGui::SameLine();
    if (ImGui::RadioButton("Pretty##t", polyscope::options::transparencyMode == polyscope::TransparencyMode::Pretty))
      polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;

    ImGui::TextUnformatted("Ground Plane:");
    if (ImGui::RadioButton("None##g", polyscope::options::groundPlaneMode == polyscope::GroundPlaneMode::None))
      polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::None;
    ImGui::SameLine();
    if (ImGui::RadioButton("Tile##g", polyscope::options::groundPlaneMode == polyscope::GroundPlaneMode::Tile))
      polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::Tile;
    if (ImGui::RadioButton("TileReflect##g",
                           polyscope::options::groundPlaneMode == polyscope::GroundPlaneMode::TileReflection))
      polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
    ImGui::SameLine();
    if (ImGui::RadioButton("ShadowOnly##g",
                           polyscope::options::groundPlaneMode == polyscope::GroundPlaneMode::ShadowOnly))
      polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;

    ImGui::TextUnformatted("SSAA:");
    ImGui::SameLine();
    if (ImGui::RadioButton("1x##ssaa", polyscope::options::ssaaFactor == 1)) polyscope::options::ssaaFactor = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("2x##ssaa", polyscope::options::ssaaFactor == 2)) polyscope::options::ssaaFactor = 2;

    if (ImGui::Checkbox("Slice Plane", &demoSlicePlaneEnabled)) {
      syncDemoSlicePlane();
    }
    ImGui::Checkbox("Show Viewport Borders", &demoShowViewportBorders);
    ImGui::Checkbox("Always Redraw (FPS test)", &polyscope::options::alwaysRedraw);
    bool shareNav = polyscope::view::getViewportsShareNavigation();
    if (ImGui::Checkbox("Share Viewport Navigation", &shareNav)) {
      polyscope::view::setViewportsShareNavigation(shareNav);
    }

    ImGui::TreePop();
  }

  ImGui::Separator();

  ImGui::InputInt("num points", &numPoints);
  ImGui::InputFloat("param value", &param);

  if (ImGui::Button("run subroutine")) {
    // mySubroutine();
  }
  ImGui::SameLine();
  if (ImGui::Button("hi")) {
    polyscope::warning("hi");
  }

  if (ImGui::Button("add implicits")) {
    addImplicitRendersFromCurrentView();
  }

  // some depth & picking stuff
  ImGui::Checkbox("test scene click", &depthClick);
  if (depthClick) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseClicked[0]) {
      glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
      int xInd, yInd;
      std::tie(xInd, yInd) = polyscope::view::screenCoordsToBufferInds(screenCoords);

      glm::vec3 worldRay = polyscope::view::screenCoordsToWorldRay(screenCoords);
      polyscope::PickResult pickResult = polyscope::pickAtScreenCoords(screenCoords);

      std::cout << "Polyscope scene test click " << std::endl;
      std::cout << "    io.MousePos.x: " << io.MousePos.x << " io.MousePos.y: " << io.MousePos.y << std::endl;
      std::cout << "    screenCoords.x: " << screenCoords.x << " screenCoords.y: " << screenCoords.y << std::endl;
      std::cout << "    bufferInd.x: " << xInd << " bufferInd.y: " << yInd << std::endl;
      std::cout << "    depth: " << pickResult.depth << std::endl;
      std::cout << "    worldRay: ";
      polyscope::operator<<(std::cout, worldRay) << std::endl;
      std::cout << "    worldPos: ";
      polyscope::operator<<(std::cout, pickResult.position) << std::endl;
      if (pickResult.isHit) {
        std::cout << "    structure: " << pickResult.structureType << " " << pickResult.structureName
                  << " local ind: " << pickResult.localIndex << std::endl;
      } else {
        std::cout << "    structure: "
                  << "none" << std::endl;
      }

      // Construct point at click location
      polyscope::registerPointCloud("click point", std::vector<glm::vec3>({pickResult.position}));

      // Construct unit-length vector pointing in the direction of the click
      // (this depends only on the camera parameters, and does not require accessing the depth buffer)
      glm::vec3 root = polyscope::view::getCameraWorldPosition();
      glm::vec3 target = root + worldRay;
      polyscope::registerCurveNetworkLine("click dir", std::vector<glm::vec3>({root, target}));

      depthClick = false;
    }
  }

  if (ImGui::Button("nested show")) {
    polyscope::show();
  }

  if (ImGui::Button("drop camera view here")) {
    dropCameraView();
  }

  if (ImGui::Button("load floating image data")) {
    loadFloatingImageData();
  }

  if (ImGui::Button("add volume grid")) {
    addVolumeGrid();
  }

  if (ImGui::Button("add sparse volume grid")) {
    addSparseVolumeGrid();
  }

  // ImPlot
  // dummy data
  if (ImGui::TreeNode("ImPlot")) {

    std::vector<float> plotVals;
    for (float t = 0; t < 10.; t += 0.01) {
      plotVals.push_back(std::cos(t + ImGui::GetTime()));
    }

    // sample plot
    if (ImPlot::BeginPlot("test plot")) {
      ImPlot::PlotLine("sample_val", &plotVals.front(), plotVals.size());
      ImPlot::EndPlot();
    }

    ImGui::TreePop();
  }

  ImGui::PopItemWidth();
}


void runMultiViewportBenchmark(const std::string& backend, int nFrames, int nWarmup) {
  polyscope::options::verbosity = 0;
  polyscope::options::maxFPS = -1;
  polyscope::options::enableVSync = false;
  polyscope::options::alwaysRedraw = true;
  polyscope::options::buildGui = false;
  polyscope::options::allowHeadlessBackends = true;
  polyscope::view::windowWidth = 1280;
  polyscope::view::windowHeight = 720;

  if (backend.empty()) {
    polyscope::init();
  } else {
    polyscope::init(backend);
  }
  polyscope::frameTick();

  auto bench = [&](const std::string& label) {
    for (int i = 0; i < nWarmup; i++) polyscope::frameTick();
    std::vector<double> times;
    times.reserve(nFrames);
    auto tStart = std::chrono::steady_clock::now();
    for (int i = 0; i < nFrames; i++) {
      auto t0 = std::chrono::steady_clock::now();
      polyscope::frameTick();
      auto t1 = std::chrono::steady_clock::now();
      times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    double totalSec = std::chrono::duration<double>(std::chrono::steady_clock::now() - tStart).count();
    std::sort(times.begin(), times.end());
    double meanMs = totalSec * 1000.0 / nFrames;
    double medMs = times[times.size() / 2];
    double fps = nFrames / totalSec;
    std::cout << std::left << std::setw(54) << label << ": " << std::right << std::fixed << std::setprecision(2)
              << std::setw(7) << meanMs << " ms mean | " << std::setw(7) << medMs << " ms med  (" << std::setw(7)
              << std::setprecision(1) << fps << " FPS)\n";
  };

  std::cout << "Window: " << polyscope::view::windowWidth << "x" << polyscope::view::windowHeight
            << ", Buffer: " << polyscope::view::bufferWidth << "x" << polyscope::view::bufferHeight << "\n"
            << std::string(96, '-') << "\n";

  // 1) Empty-scene sweep
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::None;
  for (auto mode : {polyscope::TransparencyMode::None, polyscope::TransparencyMode::Pretty}) {
    polyscope::options::transparencyMode = mode;
    polyscope::options::transparencyRenderPasses = 8;
    std::string modeName = (mode == polyscope::TransparencyMode::None) ? "none" : "pretty(8)";
    for (const auto& grid : std::vector<std::pair<int, int>>{{1, 1}, {2, 2}, {4, 3}, {8, 8}}) {
      int gw = grid.first, gh = grid.second;
      polyscope::view::createViewportGrid(gw, gh);
      bench("Empty " + std::to_string(gw) + "x" + std::to_string(gh) + " (" + std::to_string(gw * gh) + " vp), " +
            modeName + ", ground=none");
    }
  }
  std::cout << std::string(96, '-') << "\n";

  // 2) 6 spheres (4 transparent, 2 opaque) + 1 transparent point cloud
  std::vector<polyscope::SurfaceMesh*> meshes;
  std::vector<glm::vec3> V;
  std::vector<std::array<size_t, 3>> F;
  const float pi = 3.14159265358979323846f;
  for (int k = 0; k < 6; k++) {
    float ang = 2.0f * pi * k / 6.0f;
    makeSphereMesh(V, F, {0.45f * std::cos(ang), 0.3f, 0.45f * std::sin(ang)}, 0.35f, 24, 16);
    auto* m = polyscope::registerSurfaceMesh("mesh_" + std::to_string(k), V, F);
    m->setTransparency(k < 4 ? 0.5 : 1.0);
    meshes.push_back(m);
  }
  auto* cloud = registerDemoCloud("cloud_0", {0.f, 0.5f, 0.f}, 0.8f, 2000);
  cloud->setTransparency(0.7);

  // 1x1 baseline
  polyscope::view::createViewportGrid(1, 1);
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;
  bench("1x1 (7 structs), none, tile_reflection");

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  bench("1x1 (7 structs), pretty(8), tile_reflection");

  // 3x4 grid
  polyscope::view::createViewportGrid(4, 3);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
  bench("3x4 (7 structs), none, tile_reflection");

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;
  bench("3x4 (7 structs), none, shadow_only");

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  for (const auto& gpEntry : std::vector<std::pair<polyscope::GroundPlaneMode, std::string>>{
           {polyscope::GroundPlaneMode::None, "none"},
           {polyscope::GroundPlaneMode::Tile, "tile"},
           {polyscope::GroundPlaneMode::TileReflection, "tile_reflection"},
           {polyscope::GroundPlaneMode::ShadowOnly, "shadow_only"}}) {
    polyscope::options::groundPlaneMode = gpEntry.first;
    bench("3x4 (7 structs), pretty(8), ground=" + gpEntry.second);
  }

  // 3x4 (1 struct/vp)
  for (size_t i = 0; i < meshes.size(); i++) meshes[i]->setViewportVisibilityExclusive(static_cast<int>(i));
  cloud->setViewportVisibilityExclusive(6);
  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
  bench("3x4 (1 struct/vp), pretty(8), tile_reflection");

  // Overlapping insets (3 layers)
  for (auto* m : meshes) m->setViewportVisibilityAll(true);
  cloud->setViewportVisibilityAll(true);
  polyscope::view::setNumViewports(3);
  polyscope::view::activateViewport(0);
  polyscope::view::setActiveViewportRelativeLocation(0.f, 0.f);
  polyscope::view::setActiveViewportRelativeSize(1.f, 1.f);
  polyscope::view::activateViewport(1);
  polyscope::view::setActiveViewportRelativeLocation(0.64f, 0.56f);
  polyscope::view::setActiveViewportRelativeSize(0.34f, 0.40f);
  polyscope::view::activateViewport(2);
  polyscope::view::setActiveViewportRelativeLocation(0.46f, 0.36f);
  polyscope::view::setActiveViewportRelativeSize(0.28f, 0.32f);
  polyscope::view::activateViewport(0);

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;
  bench("Insets (3 layers), none, tile_reflection");

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::options::transparencyRenderPasses = 8;
  bench("Insets (3 layers), pretty(8), tile_reflection");

  polyscope::shutdown();
}

int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A simple demo of Polyscope.\nBy "
                              "Nick Sharp (nmwsharp@gmail.com)",
                              "");
  args::Flag benchFlag(parser, "bench", "Run multi-viewport render benchmark and exit", {"bench"});
  args::ValueFlag<std::string> benchBackend(parser, "backend", "Backend for --bench (e.g. openGL3_egl, openGL3_glfw)",
                                            {"bench-backend"}, "");
  args::ValueFlag<int> benchFrames(parser, "frames", "Timed frames per case for --bench", {"bench-frames"}, 40);
  args::PositionalList<std::string> files(parser, "files", "One or more files to visualize");

  // Parse args
  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Help&) {
    std::cout << parser;
    return 0;
  } catch (const args::ParseError& e) {
    std::cerr << e.what() << std::endl;

    std::cerr << parser;
    return 1;
  }

  if (benchFlag) {
    runMultiViewportBenchmark(args::get(benchBackend), args::get(benchFrames), 5);
    return 0;
  }

  // Options
  // polyscope::options::autocenterStructures = true;
  // polyscope::view::windowWidth = 600;
  // polyscope::view::windowHeight = 800;
  // polyscope::options::maxFPS = -1;
  polyscope::options::verbosity = 200;
  polyscope::options::enableRenderErrorChecks = true;
  polyscope::options::allowHeadlessBackends = true;

  // Initialize polyscope
  polyscope::init();

  // Load the default multi-viewport demo scene if no files were passed on the command line
  if (files) {
    for (std::string s : files) {
      processFile(s);
    }
  } else {
    loadViewportScene_2x2TransparentGrid();
  }

  // loadFloatingImageData();
  // addVolumeGrid();

  // Add a few gui elements
  polyscope::state::userCallback = callback;

  if (polyscope::isHeadless()) {
    // save a screenshot to prove we initialized
    std::cout << "Headless mode detected, saving screenshot" << std::endl;
    polyscope::screenshot("headless_screenshot.png");
  } else {
    // Show the gui
    polyscope::show();

    // main loop using manual frameTick() instead
    // while (!polyscope::windowRequestsClose()) {
    //   polyscope::frameTick();
    // }
  }

  std::cout << "!!!! shutdown time" << std::endl;
  polyscope::shutdown();

  return 0;
}

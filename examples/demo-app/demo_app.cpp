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
#include "polyscope/surface_mesh.h"
#include "polyscope/types.h"
#include "polyscope/view.h"
#include "polyscope/volume_grid.h"
#include "polyscope/volume_mesh.h"

#include <iostream>
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
    for (size_t iE = 0; iE < nEdges; iE++) {
      auto edge = edges[iE];
      size_t nA = std::get<0>(edge);
      size_t nB = std::get<1>(edge);

      edgeLen[iE] = glm::length(nodes[nA] - nodes[nB]);
      valEdgeCat[iE] = iE * 5 / nEdges;
      randColor[iE] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iE] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
    }
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge len", edgeLen, polyscope::DataType::MAGNITUDE);
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge categorical", valEdgeCat,
                                                                 polyscope::DataType::CATEGORICAL);
    polyscope::getCurveNetwork(curveName)->addEdgeColorQuantity("eColor", randColor);
    polyscope::getCurveNetwork(curveName)->addEdgeVectorQuantity("randVecE", randVec);
  }

  // set a node radius quantity from above
  polyscope::getCurveNetwork(curveName)->setNodeRadiusQuantity("nXabs");
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

  auto psSimpleMesh = polyscope::registerSimpleTriangleMesh(niceName, vertexPositionsGLM, faceIndices);
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
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY", valY);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cZ", valZ);
  polyscope::getSurfaceMesh(niceName)->addVertexColorQuantity("vColor", randColor);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY_sym", valY, polyscope::DataType::SYMMETRIC);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cNorm", valMag, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("categorical vert", valCat,
                                                               polyscope::DataType::CATEGORICAL);

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


void callback() {
  static int numPoints = 2000;
  static float param = 3.14;
  static int loadedMat = 1;
  static bool depthClick = false;

  ImGui::PushItemWidth(100);

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
      glm::vec3 worldPos = polyscope::view::screenCoordsToWorldPosition(screenCoords);
      std::pair<polyscope::Structure*, size_t> pickPair = polyscope::pick::pickAtScreenCoords(screenCoords);

      std::cout << "Polyscope scene test click " << std::endl;
      std::cout << "    io.MousePos.x: " << io.MousePos.x << " io.MousePos.y: " << io.MousePos.y << std::endl;
      std::cout << "    screenCoords.x: " << screenCoords.x << " screenCoords.y: " << screenCoords.y << std::endl;
      std::cout << "    bufferInd.x: " << xInd << " bufferInd.y: " << yInd << std::endl;
      std::cout << "    worldRay: ";
      polyscope::operator<<(std::cout, worldRay) << std::endl;
      std::cout << "    worldPos: ";
      polyscope::operator<<(std::cout, worldPos) << std::endl;
      if (pickPair.first == nullptr) {
        std::cout << "    structure: "
                  << "none" << std::endl;
      } else {
        std::cout << "    structure: " << pickPair.first << " element id: " << pickPair.second << std::endl;
      }

      // Construct point at click location
      polyscope::registerPointCloud("click point", std::vector<glm::vec3>({worldPos}));

      // Construct unit-length vector pointing in the direction of the click
      // (this depends only on the camera parameters, and does not require accessing the depth buffer)
      glm::vec3 root = polyscope::view::getCameraWorldPosition();
      glm::vec3 target = root + worldRay;
      polyscope::registerCurveNetworkLine("click dir", std::vector<glm::vec3>({root, target}));

      depthClick = false;
    }
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

  ImGui::PopItemWidth();
}

int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A simple demo of Polyscope.\nBy "
                              "Nick Sharp (nmwsharp@gmail.com)",
                              "");
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

  // Options
  // polyscope::options::autocenterStructures = true;
  // polyscope::view::windowWidth = 600;
  // polyscope::view::windowHeight = 800;
  // polyscope::options::maxFPS = -1;
  polyscope::options::verbosity = 100;
  polyscope::options::enableRenderErrorChecks = true;
  polyscope::options::allowHeadlessBackends = true;

  // Initialize polyscope
  polyscope::init();

  for (std::string s : files) {
    processFile(s);
  }

  // Create a point cloud
  for (int j = 0; j < 1; j++) {
    std::vector<glm::vec3> points;
    for (size_t i = 0; i < 3000; i++) {
      points.push_back(
          glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5});
    }
    polyscope::registerPointCloud("really great points" + std::to_string(j), points);
    addDataToPointCloud("really great points" + std::to_string(j), points);
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
  }
  // main loop using manual frameTick() instead
  // while (true) {
  //   polyscope::frameTick();
  // }

  std::cout << "!!!! shutdown time" << std::endl;
  polyscope::shutdown();

  return 0;
}

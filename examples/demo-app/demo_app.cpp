#include "polyscope/polyscope.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/messages.h"

#include "polyscope/curve_network.h"
#include "polyscope/point_cloud.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/surface_mesh_io.h"
#include "polyscope/file_helpers.h"

#include <iostream>
#include <unordered_set>
#include <utility>

#include "args/args.hxx"
#include "json/json.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "stb_image.h"


using std::cerr;
using std::cout;
using std::endl;
using std::string;


bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void constructDemoCurveNetwork(std::string curveName, std::vector<glm::vec3> nodes,
                               std::vector<std::array<size_t, 2>> edges) {

  // Add the curve
  if (edges.size() > 0) {
    polyscope::registerCurveNetwork(curveName, nodes, edges);
  } else {
    polyscope::registerCurveNetworkLine(curveName, nodes);
    edges = polyscope::getCurveNetwork(curveName)->edges;
  }

  // Useful data
  size_t nNodes = nodes.size();
  size_t nEdges = edges.size();

  { // Add some node values
    std::vector<double> valX(nNodes);
    std::vector<std::array<double, 3>> randColor(nNodes);
    std::vector<glm::vec3> randVec(nNodes);
    for (size_t iN = 0; iN < nNodes; iN++) {
      valX[iN] = nodes[iN].x;
      randColor[iN] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iN] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
    }
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("nX", valX);
    polyscope::getCurveNetwork(curveName)->addNodeColorQuantity("nColor", randColor);
    polyscope::getCurveNetwork(curveName)->addNodeVectorQuantity("randVecN", randVec);
  }

  { // Add some edge values
    std::vector<double> edgeLen(nEdges);
    std::vector<std::array<double, 3>> randColor(nEdges);
    std::vector<glm::vec3> randVec(nEdges);
    for (size_t iE = 0; iE < nEdges; iE++) {
      auto edge = edges[iE];
      size_t nA = std::get<0>(edge);
      size_t nB = std::get<1>(edge);

      edgeLen[iE] = glm::length(nodes[nA] - nodes[nB]);
      randColor[iE] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iE] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
    }
    polyscope::getCurveNetwork(curveName)->addEdgeScalarQuantity("edge len", edgeLen, polyscope::DataType::MAGNITUDE);
    polyscope::getCurveNetwork(curveName)->addEdgeColorQuantity("eColor", randColor);
    polyscope::getCurveNetwork(curveName)->addEdgeVectorQuantity("randVecE", randVec);
  }
}

void processFileOBJ(string filename) {
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

  // Useful data
  size_t nVertices = psMesh->nVertices();
  size_t nFaces = psMesh->nFaces();
  size_t nEdges = psMesh->nEdges();

  // Add some vertex scalars
  std::vector<double> valX(nVertices);
  std::vector<double> valY(nVertices);
  std::vector<double> valZ(nVertices);
  std::vector<double> valMag(nVertices);
  std::vector<std::array<double, 3>> randColor(nVertices);
  for (size_t iV = 0; iV < nVertices; iV++) {
    valX[iV] = vertexPositionsGLM[iV].x / 10000;
    valY[iV] = vertexPositionsGLM[iV].y;
    valZ[iV] = vertexPositionsGLM[iV].z;
    valMag[iV] = glm::length(vertexPositionsGLM[iV]);

    randColor[iV] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
  }
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cX_really_really_stupid_long_name_how_dumb", valX);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY", valY);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cZ", valZ);
  polyscope::getSurfaceMesh(niceName)->addVertexColorQuantity("vColor", randColor);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cY_sym", valY, polyscope::DataType::SYMMETRIC);
  polyscope::getSurfaceMesh(niceName)->addVertexScalarQuantity("cNorm", valMag, polyscope::DataType::MAGNITUDE);

  polyscope::getSurfaceMesh(niceName)->addVertexDistanceQuantity("cY_dist", valY);
  polyscope::getSurfaceMesh(niceName)->addVertexSignedDistanceQuantity("cY_signeddist", valY);


  // Add some face scalars
  std::vector<double> fArea(nFaces);
  std::vector<double> zero(nFaces);
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
  }
  polyscope::getSurfaceMesh(niceName)->addFaceScalarQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addFaceScalarQuantity("zero", zero);
  polyscope::getSurfaceMesh(niceName)->addFaceColorQuantity("fColor", fColor);


  // Edge length
  std::vector<double> eLen;
  std::vector<double> heLen;
  std::unordered_set<std::pair<size_t, size_t>, polyscope::hash_combine::hash<std::pair<size_t, size_t>>> seenEdges;
  for (size_t iF = 0; iF < nFaces; iF++) {
    std::vector<size_t>& face = faceIndices[iF];

    for (size_t iV = 0; iV < face.size(); iV++) {
      size_t i0 = face[iV];
      size_t i1 = face[(iV + 1) % face.size()];
      glm::vec3 p0 = vertexPositionsGLM[i0];
      glm::vec3 p1 = vertexPositionsGLM[i1];
      double len = glm::length(p0 - p1);

      size_t iMin = std::min(i0, i1);
      size_t iMax = std::max(i0, i1);

      auto p = std::make_pair(iMin, iMax);
      if (seenEdges.find(p) == seenEdges.end()) {
        eLen.push_back(len);
        seenEdges.insert(p);
      }
      heLen.push_back(len);
    }
  }
  polyscope::getSurfaceMesh(niceName)->addEdgeScalarQuantity("edge length", eLen);
  polyscope::getSurfaceMesh(niceName)->addHalfedgeScalarQuantity("halfedge length", heLen);


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
  std::vector<glm::vec3> vNormals(nVertices, glm::vec3{0., 0., 0.});
  for (size_t iF = 0; iF < nFaces; iF++) {
    std::vector<size_t>& face = faceIndices[iF];

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

    // At vertices
    std::vector<glm::vec2> vertexIntrinsicVec(nVertices, glm::vec3{0., 0., 0.});
    psMesh->ensureHaveVertexTangentSpaces();
    for (size_t iV = 0; iV < nVertices; iV++) {
      glm::vec3 pos = psMesh->vertices[iV];
      glm::vec3 basisX = psMesh->vertexTangentSpaces[iV][0];
      glm::vec3 basisY = psMesh->vertexTangentSpaces[iV][1];

      glm::vec3 v = spatialFunc(pos);
      glm::vec2 vTangent{glm::dot(v, basisX), glm::dot(v, basisY)};
      vertexIntrinsicVec[iV] = vTangent;
    }
    psMesh->addVertexIntrinsicVectorQuantity("intrinsic vertex vec", vertexIntrinsicVec);


    // At faces
    std::vector<glm::vec2> faceIntrinsicVec(nFaces, glm::vec3{0., 0., 0.});
    psMesh->ensureHaveFaceTangentSpaces();
    for (size_t iF = 0; iF < nFaces; iF++) {

      glm::vec3 pos = psMesh->faceCenter(iF);
      glm::vec3 basisX = psMesh->faceTangentSpaces[iF][0];
      glm::vec3 basisY = psMesh->faceTangentSpaces[iF][1];

      glm::vec3 v = spatialFunc(pos);
      glm::vec2 vTangent{glm::dot(v, basisX), glm::dot(v, basisY)};
      faceIntrinsicVec[iF] = vTangent;
    }
    psMesh->addFaceIntrinsicVectorQuantity("intrinsic face vec", faceIntrinsicVec);

    // 1-form
    std::vector<double> edgeForm(nEdges, 0.);
    std::vector<char> edgeOrient(nEdges, false);
    bool isTriangle = true;
    psMesh->ensureHaveFaceTangentSpaces();
    for (size_t iF = 0; iF < nFaces; iF++) {
      std::vector<size_t>& face = psMesh->faces[iF];

      if (face.size() != 3) {
        isTriangle = false;
        break;
      }

      glm::vec3 pos = psMesh->faceCenter(iF);

      for (size_t j = 0; j < face.size(); j++) {

        size_t vA = face[j];
        size_t vB = face[(j + 1) % face.size()];
        size_t iE = psMesh->edgeIndices[iF][j];

        glm::vec3 v = spatialFunc(pos);
        glm::vec3 edgeVec = psMesh->vertices[vB] - psMesh->vertices[vA];
        edgeForm[iE] = glm::dot(edgeVec, v);
        edgeOrient[iE] = (vB > vA);
      }
    }
    if (isTriangle) {
      psMesh->addOneFormIntrinsicVectorQuantity("intrinsic 1-form", edgeForm, edgeOrient);
    }
  }


  // Add count quantities
  std::vector<std::pair<size_t, int>> vCount;
  std::vector<std::pair<size_t, double>> vVal;
  for (size_t iV = 0; iV < nVertices; iV++) {
    if (polyscope::randomUnit() > 0.8) {
      vCount.push_back(std::make_pair(iV, 2));
    }
    if (polyscope::randomUnit() > 0.8) {
      vVal.push_back(std::make_pair(iV, polyscope::randomUnit()));
    }
  }
  polyscope::getSurfaceMesh(niceName)->addVertexCountQuantity("sample count", vCount);
  polyscope::getSurfaceMesh(niceName)->addVertexIsolatedScalarQuantity("sample isolated", vVal);

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

  { // Add a surface graph quantity

    std::vector<std::array<size_t, 2>> edges;
    for (size_t iF = 0; iF < nFaces; iF++) {
      std::vector<size_t>& face = faceIndices[iF];

      for (size_t iV = 0; iV < face.size(); iV++) {
        size_t i0 = face[iV];
        size_t i1 = face[(iV + 1) % face.size()];

        edges.push_back({i0, i1});
      }
    }

    polyscope::getSurfaceMesh(niceName)->addSurfaceGraphQuantity("surface graph", vertexPositionsGLM, edges);
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


  /*

  // === Input quantities
  // TODO restore

  //// Add a selection quantity
  // VertexData<char> vSelection(mesh, false);
  // for (VertexPtr v : mesh->vertices()) {
  // if (randomUnit() < 0.05) {
  // vSelection[v] = true;
  //}
  //}
  // polyscope::getSurfaceMesh(niceName)->addVertexSelectionQuantity("v select", vSelection);

  //// Curve quantity
  // polyscope::getSurfaceMesh(niceName)->addInputCurveQuantity("input curve");

  */
}

void addDataToPointCloud(string pointCloudName, const std::vector<glm::vec3>& points) {


  // Add some scalar quantities
  std::vector<double> xC(points.size());
  std::vector<std::array<double, 3>> randColor(points.size());
  for (size_t i = 0; i < points.size(); i++) {
    xC[i] = points[i].x;
    randColor[i] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
  }
  polyscope::getPointCloud(pointCloudName)->addScalarQuantity("xC", xC);
  polyscope::getPointCloud(pointCloudName)->addColorQuantity("random color", randColor);
  polyscope::getPointCloud(pointCloudName)->addColorQuantity("random color2", randColor);


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
}


void processFile(string filename) {
  // Dispatch to correct varient
  if (endsWith(filename, ".obj")) {
    processFileOBJ(filename);
  } else {
    cerr << "Unrecognized file type for " << filename << endl;
  }
}

void callback() {
  static int numPoints = 2000;
  static float param = 3.14;
  static int loadedMat = 1;

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

  ImGui::PopItemWidth();
}

int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A simple demo of Polyscope.\nBy "
                              "Nick Sharp (nsharp@cs.cmu.edu)",
                              "");
  args::PositionalList<string> files(parser, "files", "One or more files to visualize");

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
  // polyscope::options::alwaysRedraw = true;


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

  // Add a few gui elements
  polyscope::state::userCallback = callback;

  // Show the gui
  polyscope::show();

  return 0;
}

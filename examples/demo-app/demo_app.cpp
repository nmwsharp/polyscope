#include "polyscope/polyscope.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/messages.h"

#include "polyscope/camera_view.h"
#include "polyscope/curve_network.h"
#include "polyscope/file_helpers.h"
#include "polyscope/floating_quantity_structure.h"
#include "polyscope/implicit_surface.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/surface_mesh_io.h"
#include "polyscope/volume_mesh.h"

#include <iostream>
#include <unordered_set>
#include <utility>

#include "args/args.hxx"
#include "happly.h"
#include "json/json.hpp"

#include "simple_dot_mesh_parser.h"

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
  /* TODO restore

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
    std::vector<double> valXabs(nNodes);
    std::vector<std::array<double, 3>> randColor(nNodes);
    std::vector<glm::vec3> randVec(nNodes);
    for (size_t iN = 0; iN < nNodes; iN++) {
      valX[iN] = nodes[iN].x;
      valXabs[iN] = std::fabs(nodes[iN].x);
      randColor[iN] = {{polyscope::randomUnit(), polyscope::randomUnit(), polyscope::randomUnit()}};
      randVec[iN] = glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5};
    }
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("nX", valX);
    polyscope::getCurveNetwork(curveName)->addNodeScalarQuantity("nXabs", valXabs);
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

  // set a node radius quantity from above
  polyscope::getCurveNetwork(curveName)->setNodeRadiusQuantity("nXabs");
  */
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
  return; // FIXME


  // Useful data
  size_t nVertices = psMesh->nVertices();
  size_t nFaces = psMesh->nFaces();

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

  /*
  
  // size_t nEdges = psMesh->nEdges();

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

  */


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
    
    // TODO commented out for now, need to manually construct tangent bases
  
    /*

    // At vertices
    
    std::vector<glm::vec2> vertexIntrinsicVec(nVertices, glm::vec3{0., 0., 0.});
    psMesh->generateDefaultVertexTangentSpaces();
    psMesh->ensureHaveVertexTangentSpaces();
    for (size_t iV = 0; iV < nVertices; iV++) {
      glm::vec3 pos = vertexPositionsGLM[iV];
      glm::vec3 basisX = psMesh->vertexTangentSpaces[iV][0];
      glm::vec3 basisY = psMesh->vertexTangentSpaces[iV][1];

      glm::vec3 v = spatialFunc(pos);
      glm::vec2 vTangent{glm::dot(v, basisX), glm::dot(v, basisY)};
      vertexIntrinsicVec[iV] = vTangent;
    }
    psMesh->addVertexIntrinsicVectorQuantity("intrinsic vertex vec", vertexIntrinsicVec);


    // At faces
    std::vector<glm::vec2> faceIntrinsicVec(nFaces, glm::vec3{0., 0., 0.});
    psMesh->generateDefaultFaceTangentSpaces();
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
        glm::vec3 edgeVec = vertexPositionsGLM[vB] - vertexPositionsGLM[vA];
        edgeForm[iE] = glm::dot(edgeVec, v);
        edgeOrient[iE] = (vB > vA);
      }
    }
    if (isTriangle) {
      psMesh->addOneFormIntrinsicVectorQuantity("intrinsic 1-form", edgeForm, edgeOrient);
    }
    
    */
  }

  /*

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

  */

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


void loadFloatingImageData(polyscope::PointCloud* targetCloud = nullptr) {

  // load an image from disk as example data
  std::string imagePath = "test_image.png";

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

  if (targetCloud == nullptr) {
    polyscope::addColorImageQuantity("test color image", width, height, imageColor);
    polyscope::addScalarImageQuantity("test scalar image", width, height, imageScalar);

    if (hasAlpha) {
      polyscope::addColorAlphaImageQuantity("test color alpha image", width, height, imageColorAlpha);
    }
  } else {
    targetCloud->addColorImageQuantity("test color image", width, height, imageColor);
    targetCloud->addScalarImageQuantity("test scalar image", width, height, imageScalar);

    if (hasAlpha) {
      targetCloud->addColorAlphaImageQuantity("test color alpha image", width, height, imageColorAlpha);
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
  opts.mode = polyscope::ImplicitRenderMode::SphereMarch;
  opts.subsampleFactor = 2;

  polyscope::DepthRenderImageQuantity* img = polyscope::renderImplicitSurface("torus sdf", torusSDF, opts);
  polyscope::DepthRenderImageQuantity* img2 = polyscope::renderImplicitSurface("box sdf", boxFrameSDF, opts);
  polyscope::ColorRenderImageQuantity* img2Color =
      polyscope::renderImplicitSurfaceColor("box sdf color", boxFrameSDF, colorFunc, opts);
  polyscope::ScalarRenderImageQuantity* imgScalar =
      polyscope::renderImplicitSurfaceScalar("torus sdf scalar", torusSDF, scalarFunc, opts);
}

void addCameraViews() {
  polyscope::CameraView* cam1 = polyscope::registerCameraView("cam1", glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.},
                                                              glm::vec3{0., 1., 0.}, 60, 2.);
}

void processFileDotMesh(std::string filename) {
  /*
   * TODO restore
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
  */
}

void addDataToPointCloud(std::string pointCloudName, const std::vector<glm::vec3>& points) {


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

      glm::vec3 worldRay = polyscope::view::screenCoordsToWorldRay(screenCoords);
      glm::vec3 worldPos = polyscope::view::screenCoordsToWorldPosition(screenCoords);
      float depth = polyscope::view::screenCoordsToDepth(screenCoords);
      std::pair<polyscope::Structure*, size_t> pickPair =
          polyscope::pick::evaluatePickQuery(screenCoords.x, screenCoords.y);

      std::cout << "Polyscope scene test click " << std::endl;
      std::cout << "    io.MousePos.x: " << io.MousePos.x << " io.MousePos.y: " << io.MousePos.y << std::endl;
      std::cout << "    screenCoords.x: " << screenCoords.x << " screenCoords.y: " << screenCoords.y << std::endl;
      std::cout << "    worldRay: ";
      polyscope::operator<<(std::cout, worldRay) << std::endl;
      std::cout << "    worldPos: ";
      polyscope::operator<<(std::cout, worldPos) << std::endl;
      std::cout << "    depth: " << depth << std::endl;
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
      /*
      TODO restore
      glm::vec3 root = polyscope::view::getCameraWorldPosition();
      glm::vec3 target = root + worldRay;
      polyscope::registerCurveNetworkLine("click dir", std::vector<glm::vec3>({root, target}));
      */


      depthClick = false;
    }
  }


  if (ImGui::Button("add implicits")) {
    addImplicitRendersFromCurrentView();
  }

  if (ImGui::Button("add camera views")) {
    addCameraViews();
  }

  ImGui::PopItemWidth();
}

int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A simple demo of Polyscope.\nBy "
                              "Nick Sharp (nsharp@cs.cmu.edu)",
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

  // Add a few gui elements
  polyscope::state::userCallback = callback;

  // Show the gui
  polyscope::show();

  // main loop using manual frameTick() instead
  // while (true) {
  //   polyscope::frameTick();
  // }

  return 0;
}

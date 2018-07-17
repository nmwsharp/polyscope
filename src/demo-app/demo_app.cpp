#include "polyscope/polyscope.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/surface_mesh_io.h"

#include <iostream>
#include <unordered_set>
#include <utility>

#include "args/args.hxx"
#include "json/json.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using std::cerr;
using std::cout;
using std::endl;
using std::string;


bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
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
    vertexPositionsGLM.push_back({p[0], p[1], p[2]});
  }
  polyscope::registerSurfaceMesh(niceName, vertexPositionsGLM, faceIndices);

  // Useful data
  size_t nVertices = vertexPositions.size();
  size_t nFaces = faceIndices.size();

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

  // TODO show some edge and halfedge quantites
  // EdgeData<double> cWeight(mesh);
  // geom->getEdgeCotanWeights(cWeight);
  // polyscope::getSurfaceMesh(niceName)->addQuantity("cotan weight", cWeight, polyscope::DataType::SYMMETRIC);

  // HalfedgeData<double> oAngles(mesh);
  // geom->getHalfedgeAngles(oAngles);
  // polyscope::getSurfaceMesh(niceName)->addQuantity("angles", oAngles);
  // polyscope::getSurfaceMesh(niceName)->addQuantity("zangles", oAngles);

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
  // polyscope::error("Resistance is futile, welcome to the borg borg borg.");
  // polyscope::error("I'm a really, really, frustrating long error. What are you going to do with me? How ever will we
  // " "share this crisis in a way which looks right while properly wrapping text in some form or other?");
  // polyscope::terminatingError("and that was all");

  // Test warning
  polyscope::warning("Something went slightly wrong", "it was bad");

  // polyscope::warning("Smoething else went slightly wrong", "it was also bad");
  // polyscope::warning("Something went slightly wrong", "it was still bad");
  // for (int i = 0; i < 5000; i++) {
  // polyscope::warning("Some problems come in groups");
  //}

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
  polyscope::getSurfaceMesh(niceName)->addIsolatedVertexScalarQuantity("sample isolated", vVal);

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

void processFileJSON(string filename) {
  /* TODO add this back in at some point?
  using namespace nlohmann;

  std::string niceName = polyscope::guessNiceNameFromPath(filename);

  // read a JSON camera file
  std::ifstream inFile(filename);
  json j;
  inFile >> j;

  // Read the json file
  // std::vector<double> tVec = j.at("location").get<std::vector<double>>();
  // std::vector<double> rotationVec = j.at("rotation").get<std::vector<double>>();

  std::vector<double> Evec = j.at("extMat").get<std::vector<double>>();
  // std::vector<double> focalVec = j.at("focal_dists").get<std::vector<double>>();
  double fov = j.at("fov").get<double>();

  // Copy to parameters
  polyscope::CameraParameters params;
  glm::mat4x4 E;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      E[j][i] = Evec[4 * i + j]; // note: this is right because GLM uses [column][row] indexing
    }
  }
  // glm::vec2 focalLengths(.5*focalVec[0], .5*focalVec[1]); // TODO FIXME really not sure if this .5 is correct
  params.fov = fov;

  // Transform to Y-up coordinates
  glm::mat4x4 perm(0.0);
  perm[0][0] = 1.0;
  perm[2][1] = -1.0;
  perm[1][2] = 1.0;
  perm[3][3] = 1.0;

  // cout << "Perm: " << endl;
  // polyscope::prettyPrint(perm);

  params.E = E * perm;
  // params.focalLengths = focalLengths;

  polyscope::registerCameraView(niceName, params);

  // cout << "E: " << endl;
  // polyscope::prettyPrint(params.E);


  // Try to load am image right next to the camera
  std::string imageFilename = filename;
  size_t f = imageFilename.find(".json");
  imageFilename.replace(f, std::string(".json").length(), ".png");

  std::ifstream inFileIm(imageFilename);
  if (!inFileIm) {
    cout << "Did not auto-detect image at " << imageFilename << endl;
  } else {

    int x, y, n;
    unsigned char* data = stbi_load(imageFilename.c_str(), &x, &y, &n, 3);

    cout << "Loading " << imageFilename << endl;
    polyscope::getCameraView(niceName)->addImage(niceName + "_rgb", data, x, y);
  }
  */
}

void processFile(string filename) {
  // Dispatch to correct varient
  if (endsWith(filename, ".obj")) {
    processFileOBJ(filename);
  } else if (endsWith(filename, ".json")) {
    processFileJSON(filename);
  } else {
    cerr << "Unrecognized file type for " << filename << endl;
  }
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
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;

    std::cerr << parser;
    return 1;
  }

  // Options
  polyscope::options::autocenterStructures = true;

  // Initialize polyscope
  polyscope::init();

  for (std::string s : files) {
    processFile(s);
  }

  // Create a point cloud
  for (int j = 0; j < 2; j++) {
    std::vector<glm::vec3> points;
    for (size_t i = 0; i < 50; i++) {
      // points.push_back(glm::vec3{10,10,10} + 20*glm::vec3{randomUnit()-.5,
      // randomUnit()-.5, randomUnit()-.5});
      points.push_back(
          3.f * glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5});
    }
    polyscope::registerPointCloud("really great points" + std::to_string(j), points);
    addDataToPointCloud("really great points" + std::to_string(j), points);
  }

  // Add a few gui elements

  // Show the gui
  polyscope::show();

  return 0;
}

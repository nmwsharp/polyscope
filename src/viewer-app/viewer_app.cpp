#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"

#include "args/args.hxx"
#include "json/json.hpp"

using namespace geometrycentral;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

// Hack to guess the basename. Certainly does not work in all case,
// but we can just fall back on returning the full filename.
std::string guessName(std::string fullname) {
  size_t startInd = 0;
  for (std::string sep : {"/", "\\"}) {
    size_t pos = fullname.rfind(sep);
    if (pos != std::string::npos) {
      startInd = std::max(startInd, pos+1);
    }
  }

  size_t endInd = fullname.size();
  for (std::string sep : {"."}) {
    size_t pos = fullname.rfind(sep);
    if (pos != std::string::npos) {
      endInd = std::min(endInd, pos);
    }
  }

  if (startInd >= endInd) {
    return fullname;
  }

  std::string niceName = fullname.substr(startInd, endInd - startInd);
  return niceName;
}

bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void processFileOBJ(string filename) {
  // Get a nice name for the file
  std::string niceName = guessName(filename);

  Geometry<Euclidean>* geom;
  HalfedgeMesh* mesh = new HalfedgeMesh(PolygonSoupMesh(filename), geom);
  polyscope::registerSurfaceMesh(niceName, geom);

  // Add some scalars
  VertexData<double> val1(mesh);
  for(VertexPtr v : mesh->vertices()) {
    val1[v] = geom->position(v).y;
  }
  polyscope::getSurfaceMesh(niceName)->addQuantity("height", val1);

  FaceData<double> fArea(mesh);
  FaceData<double> zero(mesh);
  for(FacePtr f : mesh->faces()) {
    fArea[f] = geom->area(f);
    zero[f] = 0;
  }
  polyscope::getSurfaceMesh(niceName)->addQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addQuantity("zero", zero);
  
  EdgeData<double> cWeight(mesh);
  geom->getEdgeCotanWeights(cWeight);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cotan weight", cWeight, polyscope::DataType::SYMMETRIC);
  
  HalfedgeData<double> oAngles(mesh);
  geom->getHalfedgeAngles(oAngles);
  polyscope::getSurfaceMesh(niceName)->addQuantity("angles", oAngles);
  
  
  // Add some vectors
  VertexData<Vector3> normals(mesh);
  VertexData<Vector3> toZero(mesh);
  geom->getVertexNormals(normals);
  for(VertexPtr v : mesh->vertices()) {
    normals[v] *= unitRand() * 5000;
    toZero[v] = -geom->position(v);
  }
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("normals", normals);
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("toZero", toZero, polyscope::VectorType::AMBIENT);

  FaceData<Vector3> fNormals(mesh);
  for(FacePtr f : mesh->faces()) {
    fNormals[f] = geom->normal(f);
  }
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("face normals", fNormals);

  delete geom;
  delete mesh;
}

void processFileJSON(string filename) {

  using namespace nlohmann;

  std::string niceName = guessName(filename);

  // read a JSON camera file
  std::ifstream inFile(filename);
  json j;
  inFile >> j;

  // Read the json file
  // std::vector<double> tVec = j.at("location").get<std::vector<double>>();
  // std::vector<double> rotationVec = j.at("rotation").get<std::vector<double>>();
  
  std::vector<double> Evec = j.at("extMat").get<std::vector<double>>();
  std::vector<double> focalVec = j.at("focal_dists").get<std::vector<double>>();
 
  // Copy to parameters
  polyscope::CameraParameters params;
  // glm::mat3x3 R;
  // glm::vec3 t;
  glm::mat4x4 E;
  // for(int i = 0; i < 3; i++) {
  //   t[i] = tVec[i];
  //   for(int j = 0; j < 3; j++) {
  //     R[i][j] = rotationVec[3*i + j];
  //   }
  // }
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      E[i][j] = Evec[4*i + j];
    }
  }
  glm::vec2 focalLengths(focalVec[0], focalVec[1]);

  // TODO not right, different conventions
  // params.T = t;
  // params.R = R;
  params.E = E;
  params.focalLengths = focalLengths;

  polyscope::registerCameraView(niceName, params);

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
  args::ArgumentParser parser(
      "A general purpose viewer for geometric data, built on Polyscope.\nBy "
      "Nick Sharp (nsharp@cs.cmu.edu)",
      "");
  args::PositionalList<string> files(parser, "files",
                                     "One or more files to visualize");

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

  // Initialize polyscope
  polyscope::init();

  for (std::string s : files) {
    processFile(s);
  }

  // Create a point cloud
  std::vector<Vector3> points;
  for (size_t i = 0; i < 3000; i++) {
    // points.push_back(Vector3{10,10,10} + 20*Vector3{unitRand()-.5,
    // unitRand()-.5, unitRand()-.5});
    points.push_back(
        3 * Vector3{unitRand() - .5, unitRand() - .5, unitRand() - .5});
  }
  polyscope::registerPointCloud("really great points", points);

  // Create a camera view
  polyscope::CameraParameters pTest;
  polyscope::registerCameraView("testview 1", pTest);

  // Add a few gui elements

  // Show the gui
  polyscope::show();

  return 0;
}
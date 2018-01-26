#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"

#include "args/args.hxx"
#include "json/json.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace geometrycentral;
using std::cerr;
using std::cout;
using std::endl;
using std::string;


bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void processFileOBJ(string filename) {
  // Get a nice name for the file
  std::string niceName = polyscope::utilities::guessNiceNameFromPath(filename);

  Geometry<Euclidean>* geom;
  HalfedgeMesh* mesh = new HalfedgeMesh(PolygonSoupMesh(filename), geom);
  polyscope::registerSurfaceMesh(niceName, geom);

  // Add some scalars
  VertexData<double> valX(mesh);
  VertexData<double> valY(mesh);
  VertexData<double> valZ(mesh);
  VertexData<double> valMag(mesh);
  VertexData<Vector3> randColor(mesh);
  for (VertexPtr v : mesh->vertices()) {
    valX[v] = geom->position(v).x/10000;
    valY[v] = geom->position(v).y;
    valZ[v] = geom->position(v).z;
    valMag[v] = norm(geom->position(v));

    randColor[v] = Vector3{unitRand(), unitRand(), unitRand()};
  }
  polyscope::getSurfaceMesh(niceName)->addQuantity("cX_really_really_stupid_long_name_how_dumb", valX);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cY", valY);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cZ", valZ);
  polyscope::getSurfaceMesh(niceName)->addColorQuantity("vColor", randColor);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cY_sym", valY, polyscope::DataType::SYMMETRIC);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cNorm", valMag, polyscope::DataType::MAGNITUDE);

  FaceData<double> fArea(mesh);
  FaceData<double> zero(mesh);
  FaceData<Vector3> fColor(mesh);
  for (FacePtr f : mesh->faces()) {
    fArea[f] = geom->area(f);
    zero[f] = 0;
    fColor[f] = Vector3{unitRand(), unitRand(), unitRand()};
  }
  polyscope::getSurfaceMesh(niceName)->addQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);
  polyscope::getSurfaceMesh(niceName)->addQuantity("zero", zero);
  polyscope::getSurfaceMesh(niceName)->addColorQuantity("fColor", fColor);

  EdgeData<double> cWeight(mesh);
  geom->getEdgeCotanWeights(cWeight);
  polyscope::getSurfaceMesh(niceName)->addQuantity("cotan weight", cWeight, polyscope::DataType::SYMMETRIC);

  HalfedgeData<double> oAngles(mesh);
  geom->getHalfedgeAngles(oAngles);
  polyscope::getSurfaceMesh(niceName)->addQuantity("angles", oAngles);

  // Test error
  // polyscope::error("Resistance is futile, welcome to the borg borg borg.");
  // polyscope::error("I'm a really, really, frustrating long error. What are you going to do with me? How ever will we
  // share this crisis in a way which looks right while properly wrapping text in some form or other?");
  // polyscope::terminatingError("and that was all");

  // Test warning
  // polyscope::warning("Something went slightly wrong", "it was bad");
  // polyscope::warning("Smoething else went slightly wrong", "it was also bad");
  // polyscope::warning("Something went slightly wrong", "it was still bad");
  // for(int i = 0; i < 5000; i++) {
  // polyscope::warning("Some problems come in groups");
  //}

  // Add some vectors
  VertexData<Vector3> normals(mesh);
  VertexData<Vector3> toZero(mesh);
  geom->getVertexNormals(normals);
  for (VertexPtr v : mesh->vertices()) {
    normals[v] *= unitRand() * 5000;
    toZero[v] = -geom->position(v);
  }
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("rand length normals", normals);
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("toZero", toZero, polyscope::VectorType::AMBIENT);

  FaceData<Vector3> fNormals(mesh);
  for (FacePtr f : mesh->faces()) {
    fNormals[f] = geom->normal(f);
  }
  polyscope::getSurfaceMesh(niceName)->addVectorQuantity("face normals", fNormals);

  delete geom;
  delete mesh;
}

void processFileJSON(string filename) {

  using namespace nlohmann;

  std::string niceName = polyscope::utilities::guessNiceNameFromPath(filename);

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
  args::ArgumentParser parser("A general purpose viewer for geometric data, built on Polyscope.\nBy "
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

  // Initialize polyscope
  polyscope::init();

  for (std::string s : files) {
    processFile(s);
  }

  // Create a point cloud
  for (int j = 0; j < 2; j++) {
    std::vector<Vector3> points;
    for (size_t i = 0; i < 50; i++) {
      // points.push_back(Vector3{10,10,10} + 20*Vector3{unitRand()-.5,
      // unitRand()-.5, unitRand()-.5});
      points.push_back(3 * Vector3{unitRand() - .5, unitRand() - .5, unitRand() - .5});
    }
    polyscope::registerPointCloud("really great points" + std::to_string(j), points);
  }

  // Add a few gui elements

  // Show the gui
  polyscope::show();

  return 0;
}

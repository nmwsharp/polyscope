#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"

#include "args/args.hxx"

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

  delete geom;
  delete mesh;
}

void processFile(string filename) {
  // Dispatch to correct varient
  if (endsWith(filename, ".obj")) {
    processFileOBJ(filename);
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
//   std::vector<Vector3> points;
//   for (size_t i = 0; i < 3000; i++) {
//     // points.push_back(Vector3{10,10,10} + 20*Vector3{unitRand()-.5,
//     // unitRand()-.5, unitRand()-.5});
//     points.push_back(
//         3 * Vector3{unitRand() - .5, unitRand() - .5, unitRand() - .5});
//   }
//   polyscope::registerPointCloud("really great points", points);

  // Add a few gui elements

  // Show the gui
  polyscope::show();

  return 0;
}
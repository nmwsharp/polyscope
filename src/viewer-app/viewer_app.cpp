#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"
#include "geometrycentral/ply_wrapper.h"

#include "args/args.hxx"

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

  cout << "Reading obj file " << filename << endl;

  // Get a nice name for the file
  std::string niceName = polyscope::utilities::guessNiceNameFromPath(filename);

  Geometry<Euclidean>* geom;
  HalfedgeMesh* mesh = new HalfedgeMesh(PolygonSoupMesh(filename), geom);
  polyscope::registerSurfaceMesh(niceName, geom);

  // TODO texcoords?

  delete geom;
  delete mesh;
}

void processFilePLY(string filename) {
  
  cout << "Reading ply file " << filename << endl;

  // Get a nice name for the file
  std::string niceName = polyscope::utilities::guessNiceNameFromPath(filename);

  // Initialize a PLY reader
  PlyHalfedgeMeshData reader(filename, true);
    
  Geometry<Euclidean>* geom = reader.getMesh();
  HalfedgeMesh* mesh = geom->getMesh();
  polyscope::registerSurfaceMesh(niceName, geom);

  // Try to get vertex colors, if present
  try {
    VertexData<Vector3> color = reader.getVertexColors();
    polyscope::getSurfaceMesh(niceName)->addColorQuantity("vertex color", color);
  } catch (...) {};

  // Try to get UV coordinates, if present

  delete geom->getMesh();
  delete geom;
}


void processFile(string filename) {
  // Dispatch to correct varient
  if (endsWith(filename, ".obj")) {
    processFileOBJ(filename);
  } else if (endsWith(filename, ".ply")) {
    processFilePLY(filename);
  } else {
    cerr << "Unrecognized file type for " << filename << endl;
  }
}

int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A general purpose viewer for geometric data (obj, ply, etc), built on Polyscope.\nBy "
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

  // Show the gui
  polyscope::show();

  return 0;
}

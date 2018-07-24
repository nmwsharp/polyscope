#include "polyscope/polyscope.h"


#include "polyscope/surface_mesh_io.h"

#include <iostream>

#include "args/args.hxx"

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

void processFileOBJ(string filename) {

  cout << "Reading obj file " << filename << endl;

  // Get a nice name for the file
  std::string niceName = polyscope::guessNiceNameFromPath(filename);

  std::vector<std::array<double, 3>> vertexPositions;
  std::vector<std::vector<size_t>> faceIndices;
  polyscope::loadPolygonSoup_OBJ(filename, vertexPositions, faceIndices);
  
  
  std::vector<glm::vec3> vertexPositionsGLM;
  for(std::array<double, 3> p : vertexPositions) {
    vertexPositionsGLM.push_back({p[0], p[1], p[2]});
  }

  polyscope::registerSurfaceMesh(niceName, vertexPositionsGLM, faceIndices);

  // TODO texcoords?
}

void processFilePLY(string filename) {

  cout << "Reading ply file " << filename << endl;

  // Get a nice name for the file
  std::string niceName = polyscope::guessNiceNameFromPath(filename);

  std::vector<std::array<double, 3>> vertexPositions;
  std::vector<std::vector<size_t>> faceIndices;
  polyscope::loadPolygonSoup_PLY(filename, vertexPositions, faceIndices);
  
  std::vector<glm::vec3> vertexPositionsGLM;
  for(std::array<double, 3> p : vertexPositions) {
    vertexPositionsGLM.push_back({p[0], p[1], p[2]});
  }

  polyscope::registerSurfaceMesh(niceName, vertexPositionsGLM, faceIndices);

  // Try to get vertex colors, if present
  //try {
    //VertexData<Vector3> color = reader.getVertexColors();
    //polyscope::getSurfaceMesh(niceName)->addColorQuantity("vertex color", color);
  //} catch (...) {
  //};

  // Try to get UV coordinates, if present
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

  // Initialize polyscope
  polyscope::init();

  for (std::string s : files) {
    processFile(s);
  }

  // Show the gui
  polyscope::show();

  return 0;
}

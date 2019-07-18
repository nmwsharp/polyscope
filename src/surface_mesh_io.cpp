// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_mesh_io.h"

#include "polyscope/messages.h"

#include "happly.h"

#include <algorithm>

namespace polyscope {


// Helpers for the OBJ reader
// TODO factor this out in to something neat like happly
namespace {

class Index {
public:
  Index() {}

  Index(int v, int vt, int vn) : position(v), uv(vt), normal(vn) {}

  bool operator<(const Index& i) const {
    if (position < i.position) return true;
    if (position > i.position) return false;
    if (uv < i.uv) return true;
    if (uv > i.uv) return false;
    if (normal < i.normal) return true;
    if (normal > i.normal) return false;

    return false;
  }

  int position;
  int uv;
  int normal;
};

Index parseFaceIndex(const std::string& token) {
  std::stringstream in(token);
  std::string indexString;
  int indices[3] = {1, 1, 1};

  int i = 0;
  while (std::getline(in, indexString, '/')) {
    if (indexString != "\\") {
      std::stringstream ss(indexString);
      ss >> indices[i++];
    }
  }

  // decrement since indices in OBJ files are 1-based
  return Index(indices[0] - 1, indices[1] - 1, indices[2] - 1);
}
} // namespace


void loadPolygonSoup_OBJ(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                         std::vector<std::vector<size_t>>& faceIndicesOut) {


  faceIndicesOut.clear();
  vertexPositionsOut.clear();

  // Open the file
  std::ifstream in(filename);
  if (!in) throw std::invalid_argument("Could not open mesh file " + filename);

  // parse obj format
  std::string line;
  while (getline(in, line)) {
    std::stringstream ss(line);
    std::string token;

    ss >> token;

    if (token == "v") {
      double x, y, z;
      ss >> x >> y >> z;

      vertexPositionsOut.push_back({{x, y, z}});

    } else if (token == "vt") {
      // Do nothing

    } else if (token == "vn") {
      // Do nothing

    } else if (token == "f") {
      std::vector<size_t> face;
      while (ss >> token) {
        Index index = parseFaceIndex(token);
        if (index.position < 0) {
          getline(in, line);
          size_t i = line.find_first_not_of("\t\n\v\f\r ");
          index = parseFaceIndex(line.substr(i));
        }

        face.push_back(index.position);
      }

      faceIndicesOut.push_back(face);
    }
  }
}

void loadPolygonSoup_PLY(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                         std::vector<std::vector<size_t>>& faceIndicesOut) {

  happly::PLYData plyIn(filename);

  // Get mesh-style data from the object
  vertexPositionsOut = plyIn.getVertexPositions();
  faceIndicesOut = plyIn.getFaceIndices<size_t>();
}

void loadPolygonSoup(std::string filename, std::vector<std::array<double, 3>>& vertexPositionsOut,
                     std::vector<std::vector<size_t>>& faceIndicesOut) {

  // Check if file exists
  std::ifstream testStream(filename);
  if (!testStream) {
    error("Could not load polygon soup; file does not exist: " + filename);
    return;
  }
  testStream.close();

  // Attempt to detect filename
  std::string::size_type sepInd = filename.rfind('.');
  std::string extension = "";
  if (sepInd != std::string::npos) {
    extension = filename.substr(sepInd + 1);

    // Convert to all lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  }


  if (extension == "obj") {
    loadPolygonSoup_OBJ(filename, vertexPositionsOut, faceIndicesOut);
  } else if (extension == "ply") {
    loadPolygonSoup_PLY(filename, vertexPositionsOut, faceIndicesOut);
  } else {
    error("Could not detect file type to load mesh from " + filename);
  }
}
} // namespace polyscope

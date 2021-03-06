#include "simple_dot_mesh_parser.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>

void parseVolumeDotMesh(std::string filename, std::vector<std::array<double, 3>>& vertsOut,
                        std::vector<std::array<int64_t, 8>>& cellsOut) {

  // Open the file
  std::ifstream in(filename);
  if (!in) throw std::invalid_argument("Could not open mesh file " + filename);

  vertsOut.clear();
  cellsOut.clear();

  // std::stringstream ss(line);
  // parse
  std::string line;
  while (getline(in, line)) {

    if (line == "MeshVersionFormatted 1") {
    } else if (line == "Dimension 3") {
    } else if (line == "End") {
      return;
    } else if (line == "Vertices") {
      getline(in, line);
      std::stringstream ss(line);
      size_t nVerts;
      ss >> nVerts;

      vertsOut.resize(nVerts);
      for (size_t iVert = 0; iVert < nVerts; iVert++) {
        getline(in, line);
        std::stringstream ss(line);
        double x, y, z;
        ss >> x >> y >> z;
        vertsOut[iVert][0] = x;
        vertsOut[iVert][1] = y;
        vertsOut[iVert][2] = z;
      }
    } else if (line == "Hexahedra") {
      getline(in, line);
      std::stringstream ss(line);
      size_t nHex;
      ss >> nHex;
      for (size_t iHex = 0; iHex < nHex; iHex++) {
        getline(in, line);
        std::stringstream ss(line);
        std::array<int64_t, 8> cell;
        for (int j = 0; j < 8; j++) {
          int64_t ind;
          ss >> ind;
          cell[j] = ind - 1;
        }
        cellsOut.push_back(cell);
      }
    }
  }
}

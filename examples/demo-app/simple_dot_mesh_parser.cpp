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

  while (in) {

    std::string token;
    in >> token;

    if (token == "MeshVersionFormatted") {
      in >> token; // eat version number
    } else if (token == "Dimension") {
      in >> token; // eat dimension number
    } else if (token == "End") {
      return;
    } else if (token == "Vertices") {
      size_t nVerts;
      in >> nVerts;

      vertsOut.resize(nVerts);
      for (size_t iVert = 0; iVert < nVerts; iVert++) {
        double x, y, z;
        int value;
        in >> x >> y >> z >> value;
        vertsOut[iVert][0] = x;
        vertsOut[iVert][1] = y;
        vertsOut[iVert][2] = z;
        // not sure what value even does
      }
    } else if (token == "Tetrahedra") {
      size_t nTet;
      in >> nTet;
      for (size_t iTet = 0; iTet < nTet; iTet++) {
        std::array<int64_t, 8> cell;
        int value;
        for (int j = 0; j < 4; j++) {
          int64_t ind;
          in >> ind;
          cell[j] = ind - 1;
        }
        for (int j = 4; j < 8; j++) {
          cell[j] = -1;
        }
        in >> value;
        cellsOut.push_back(cell);
      }
    } else if (token == "Hexahedra") {
      size_t nHex;
      in >> nHex;
      for (size_t iHex = 0; iHex < nHex; iHex++) {
        std::array<int64_t, 8> cell;
        int value;
        for (int j = 0; j < 8; j++) {
          int64_t ind;
          in >> ind;
          cell[j] = ind - 1;
        }
        in >> value;
        cellsOut.push_back(cell);
      }
    }
  }
}

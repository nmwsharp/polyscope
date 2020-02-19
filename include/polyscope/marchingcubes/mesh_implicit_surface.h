#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "CIsoSurface.h"

namespace polyscope {
namespace marchingcubes {

template <typename Implicit, typename Data>
void SampleFunctionToGrid(const Implicit &funct, size_t numCornersPerSide, glm::vec3 center, double sideLength, std::vector<Data> &field) {
  double diameter = sideLength;
  double cellSize = diameter / (numCornersPerSide - 1);
  double radius = diameter / 2;

  glm::vec3 lowerCorner = center - glm::vec3{radius, radius, radius};

  int nSlice = numCornersPerSide * numCornersPerSide;
  int nRow = numCornersPerSide;

  for (size_t x = 0; x < numCornersPerSide; x++) {
    for (size_t y = 0; y < numCornersPerSide; y++) {
      for (size_t z = 0; z < numCornersPerSide; z++) {
        glm::vec3 samplePt = lowerCorner + glm::vec3{(double)x, (double)y, (double)z} * (float)cellSize;
        Data value = funct.ValueAt(samplePt);
        field[nSlice * z + nRow * y + x] = value;
      }
    }
  }
}

inline void MeshImplicitGrid(std::vector<double> &field, double isoLevel, size_t numCornersPerSide, glm::vec3 center, double sideLength,
                             std::vector<glm::vec3> &nodes, std::vector<std::array<size_t, 3>> &triangles) {
  CIsoSurface<double>* iso = new CIsoSurface<double>();
  int numCells = numCornersPerSide - 1;
  double diameter = sideLength;
  double cellSize = diameter / numCells;
  double radius = diameter / 2;
  glm::vec3 lowerCorner = center - glm::vec3{radius, radius, radius};

  iso->GenerateSurface(&field[0], isoLevel, numCells, numCells, numCells, cellSize, cellSize, cellSize);

  int nVerts = iso->m_nVertices;

  for (int i = 0; i < nVerts; i++) {
    double x = iso->m_ppt3dVertices[i][0];
    double y = iso->m_ppt3dVertices[i][1];
    double z = iso->m_ppt3dVertices[i][2];

    glm::vec3 p = lowerCorner + glm::vec3{x, y, z};
    nodes.push_back(p);
  }

  int nTris = iso->m_nTriangles;

  for (int i = 0; i < nTris; i++) {
    int i1 = iso->m_piTriangleIndices[3 * i];
    int i2 = iso->m_piTriangleIndices[3 * i + 1];
    int i3 = iso->m_piTriangleIndices[3 * i + 2];

    triangles.push_back({(size_t)i1, (size_t)i2, (size_t)i3});
  }
  delete iso;
}

} // namespace marchingcubes
} // namespace polyscope
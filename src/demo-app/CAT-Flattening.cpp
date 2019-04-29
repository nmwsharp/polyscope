#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"
#include "geometrycentral/linear_solvers.h"

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
void solveOptMatrix(Geometry<Euclidean>* geom, HalfedgeMesh* mesh, std::string niceName)
{
  // Initialize basic information
  VertexData<size_t> vInd = mesh->getVertexIndices();
  size_t nVerts = mesh->nVertices();
  HalfedgeData<size_t> hInd = mesh->getHalfedgeIndices();
  size_t nHalfedges = mesh->nHalfedges();
  size_t dim = nVerts + nHalfedges;

  VertexData<double> angleDefects(mesh);
  geom->getVertexAngleDefects(angleDefects);

  EdgeData<double> lengths(mesh);
  geom->getEdgeLengths(lengths);

  HalfedgeData<double> finalCurvature(mesh);
  VertexData<double> multiplier(mesh);

  Eigen::SparseMatrix<double> d0 = Eigen::SparseMatrix<double>(dim, dim);
  Vector<double> rhs = Vector<double>(dim);

  for (size_t i = 0; i < nHalfedges; i ++)
  {
    d0.insert(i,i) =  1.;
    rhs[i] = 0.;
  }

  for (size_t i = nHalfedges; i < dim; i++)
  {
    rhs[i] = 2 * angleDefects[mesh->vertex(i - nHalfedges)];
  }
  for (EdgePtr e : mesh->edges())
  {
    HalfedgePtr h1 = e.halfedge();
    HalfedgePtr h2 = h1.twin();
    size_t v1 = vInd[h1.vertex()];
    size_t v2 = vInd[h2.vertex()];
    d0.insert(nHalfedges + v1, hInd[h1]) = lengths[e];
    d0.insert(nHalfedges + v1, hInd[h2]) = lengths[e];
    d0.insert(nHalfedges + v2, hInd[h1]) = lengths[e];
    d0.insert(nHalfedges + v2, hInd[h2]) = lengths[e];

    d0.insert(hInd[h1], nHalfedges + v1) = lengths[e];
    d0.insert(hInd[h2], nHalfedges + v1) = lengths[e];
    d0.insert(hInd[h1], nHalfedges + v2) = lengths[e];
    d0.insert(hInd[h2], nHalfedges + v2) = lengths[e];
  }
  cout << "Matrix built";
  Vector<double> solution = solve(d0 ,rhs);
  cout << "Matrix solved";
  for (size_t i = 0; i < nHalfedges; i++)
  {
    finalCurvature[i] = solution[i];
  }
  for (size_t i = nHalfedges; i < dim; i++)
  {
    multiplier[i] = solution[i];
  }
  polyscope::getSurfaceMesh(niceName)->addQuantity("Curvature change", finalCurvature);
  polyscope::getSurfaceMesh(niceName)->addQuantity("Lagrange Multiplier", multiplier);
  return;
}


void processFileOBJ(string filename) {
  // Get a nice name for the file
  std::string niceName = polyscope::utilities::guessNiceNameFromPath(filename);

  Geometry<Euclidean>* geom;
  HalfedgeMesh* mesh = new HalfedgeMesh(PolygonSoupMesh(filename), geom);
  polyscope::registerSurfaceMesh(niceName, geom);
  solveOptMatrix(geom, mesh, niceName);

  delete geom;
  delete mesh;
}


int main(int argc, char** argv) {
  // Configure the argument parser
  args::ArgumentParser parser("A simple demo of Polyscope.\nBy "
                              "Nick Sharp (nsharp@cs.cmu.edu)",
                              "");
  args::PositionalList<string> files(parser, "files", "One or more files to visualize");
  // Options
  polyscope::options::autocenterStructures = true;
  // Initialize polyscope
  polyscope::init();
  processFileOBJ("C:/spot.obj");
  // Show the gui
  polyscope::show();

  return 0;
}

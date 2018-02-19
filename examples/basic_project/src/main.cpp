#include "polyscope/polyscope.h"

#include <iostream>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/polygon_soup_mesh.h"
#include "geometrycentral/direction_fields.h"

#include "imgui.h"
#include "args/args.hxx"

using namespace geometrycentral;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

// == Program data
// (in general, such data should probably be stored in a class, or whatever makes sense for your situation -- these globals are just for the sake of a simple example app)
Geometry<Euclidean>* geom;
HalfedgeMesh* mesh;

// Parameters 
size_t iGeneratedPoints = 0;
int nPts = 100;
float rangeLow = -5.0;
float rangeHigh = 5.0;

// A user-defined callback, for creating control panels (etc)
// Use ImGUI commands to build whatever you want here, see https://github.com/ocornut/imgui/blob/master/imgui.h
void myCallback() {

  // Begin an ImGUI window
  static bool showGui = true;
  ImGui::Begin("Sample UI", &showGui, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::PushItemWidth(100);

  // Generate a random function
  ImGui::TextUnformatted("Generate random function:");
  ImGui::DragFloatRange2("Data range", &rangeLow, &rangeHigh);
  if (ImGui::Button("Generate")) {
    VertexData<double> randF(mesh);
    for (VertexPtr v : mesh->vertices()) {
      randF[v] = randomReal(rangeLow, rangeHigh);
    }
    polyscope::getSurfaceMesh()->addQuantity("generated_function", randF);
  }
  ImGui::Separator();


  // Add points 
  ImGui::TextUnformatted("Add new points clouds:");
  ImGui::InputInt("# pts", &nPts, 0, 1000000);
  if (ImGui::Button("Add another")) {
    std::vector<Vector3> points;
    for (int i = 0; i < nPts; i++) {
      points.push_back(3 * Vector3{unitRand() - .5, unitRand() - .5, unitRand() - .5});
    }
    polyscope::registerPointCloud("generated_points_"+std::to_string(iGeneratedPoints), points);
    iGeneratedPoints++;
  }
  ImGui::Separator();

  if (ImGui::Button("Batman")) {
    polyscope::warning("Na na na na na na na na na na na na na Batman!");
  }

  // Cleanup the ImGUI window
  ImGui::PopItemWidth();
  ImGui::End();
}

int main(int argc, char** argv) {

  // Configure the argument parser
  args::ArgumentParser parser("Polyscope sample program. See github.com/nmwsharp/polyscope/examples.");
  args::Positional<string> inFileName(parser, "input_file", "An .obj file to visualize");

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

  // Make sure a mesh name was given
  if(args::get(inFileName) == "") {
    std::cerr << "Please specify .obj file as argument" << std::endl;
    return EXIT_FAILURE;
  }

  // Initialize polyscope
  polyscope::init();

  // == Build the mesh object from the input file
  mesh = new HalfedgeMesh(PolygonSoupMesh(args::get(inFileName)), geom);
  std::string meshNiceName = polyscope::utilities::guessNiceNameFromPath(args::get(inFileName));
  polyscope::registerSurfaceMesh(meshNiceName, geom);

  // == Add some data to the mesh we just created
  // Note: Since the viewer only currently only has one mesh, we can omit the mesh name field
  //       from these commands -- otherwise a correct name must be specified to getSurfaceMesh()
  {
    // Two function on vertices (x coord and a random color)
    VertexData<double> valX(mesh);
    VertexData<Vector3> randColor(mesh);
    for (VertexPtr v : mesh->vertices()) {
      valX[v] = geom->position(v).x;
      randColor[v] = Vector3{unitRand(), unitRand(), unitRand()};
    }
    polyscope::getSurfaceMesh()->addQuantity("x coord", valX);
    polyscope::getSurfaceMesh()->addColorQuantity("random color", randColor);

    // Face area
    FaceData<double> fArea(mesh);
    for (FacePtr f : mesh->faces()) {
      fArea[f] = geom->area(f);
    }
    polyscope::getSurfaceMesh()->addQuantity("face area", fArea, polyscope::DataType::MAGNITUDE);

    // Edge cotan weights
    EdgeData<double> cWeight(mesh);
    geom->getEdgeCotanWeights(cWeight);
    polyscope::getSurfaceMesh()->addQuantity("cotan weight", cWeight, polyscope::DataType::SYMMETRIC);
 
    // Vertex normals
    VertexData<Vector3> normals(mesh);
    geom->getVertexNormals(normals);
    polyscope::getSurfaceMesh()->addVectorQuantity("vertex normals", normals);

    // Smoothest 4-symmetric direction field
    if(mesh->nBoundaryLoops() == 0) { // (haven't implemented for boundary yet...)
      FaceData<Complex> smoothestField = computeSmoothestFaceDirectionField(geom, 4, true);
      polyscope::getSurfaceMesh()->addVectorQuantity("smoothest 4-field", smoothestField, 4);
    }
  }

  // == Create a point cloud
  std::vector<Vector3> points;
  for (size_t i = 0; i < 50; i++) {
    points.push_back(3 * Vector3{unitRand() - .5, unitRand() - .5, unitRand() - .5});
  }
  polyscope::registerPointCloud("sample_points", points);


  // Register the user callback 
  polyscope::state::userCallback = myCallback;

  // Give control to the polyscope gui
  polyscope::show();

  return EXIT_SUCCESS;
}

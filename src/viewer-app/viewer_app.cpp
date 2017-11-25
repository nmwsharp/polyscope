#include "polyscope/polyscope.h"

#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/geometry.h"
#include "geometrycentral/polygon_soup_mesh.h"

using namespace geometrycentral;

int main(int argc, char** argv) {
    

    // Initialize polyscope
    polyscope::init();

    // Create a point cloud
    std::vector<Vector3> points;
    for(size_t i = 0; i < 3000; i++) {
    // for(size_t i = 0; i < 1; i++) {
        // points.push_back(Vector3{10,10,10} + 20*Vector3{unitRand()-.5, unitRand()-.5, unitRand()-.5});
        points.push_back(3*Vector3{unitRand()-.5, unitRand()-.5, unitRand()-.5});
    }

    // Load the point cloud in to polyscope
    polyscope::registerPointCloud("really great points", points);

    // Read a mesh
    Geometry<Euclidean>* geom1;
    Geometry<Euclidean>* geom2;
    HalfedgeMesh* mesh1 = new HalfedgeMesh(PolygonSoupMesh("/Users/nsharp/mesh/sphere_medium.obj"), geom1);
    HalfedgeMesh* mesh2 = new HalfedgeMesh(PolygonSoupMesh("/Users/nsharp/mesh/armadillo.obj"), geom2);
    // HalfedgeMesh* mesh = new HalfedgeMesh(PolygonSoupMesh("/Users/nsharp/mesh/raptor_orig.obj"), geom);
    polyscope::registerSurfaceMesh("mr mesh1", geom1);
    polyscope::registerSurfaceMesh("mr mesh2", geom2);

    // Load the mesh in to polyscope

    // Add a few gui elements

    // Show the gui
    polyscope::show();

    return 0;

}
#include "polyscope/polyscope.h"

#include "geometrycentral/halfedge_mesh.h"

using namespace geometrycentral;

int main(int argc, char** argv) {
    
    geometrycentral::HalfedgeMesh* mesh = new geometrycentral::HalfedgeMesh();
    VertexData<double> vd(mesh);

    // Initialize polyscope
    polyscope::init();

    // Create a point cloud
    std::vector<Vector3> points;
    for(size_t i = 0; i < 300; i++) {
        points.push_back(Vector3{unitRand(), unitRand(), unitRand()});
    }

    // Load the point cloud in to polyscope
    polyscope::registerPointCloud("really great points", points);


    // Read a mesh

    // Load the mesh in to polyscope

    // Add a few gui elements

    // Show the gui
    polyscope::show();

    return 0;

}
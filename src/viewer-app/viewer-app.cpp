#include "polyscope/polyscope.h"

#include "geometrycentral/halfedge_mesh.h"

using namespace geometrycentral;

int main(int argc, char** argv) {
    
    geometrycentral::HalfedgeMesh* mesh = new geometrycentral::HalfedgeMesh();
    VertexData<double> vd(mesh);

    // Initialize polyscope
    polyscope::init();

    // Read a mesh

    // Load the mesh in to polyscope

    // Add a few gui elements

    // Show the gui
    polyscope::show();

    return 0;

}
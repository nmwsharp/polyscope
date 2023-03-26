#include "polyscope/polyscope.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/messages.h"

#include "polyscope/curve_network.h"
#include "polyscope/file_helpers.h"
#include "polyscope/point_cloud.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/surface_mesh_io.h"
#include "polyscope/volume_mesh.h"

#include <iostream>
#include <unordered_set>
#include <utility>

#include "args/args.hxx"
#include "happly.h"
#include "json/json.hpp"

#include "cuda_interop_demo.h"
  

// Working data
int nPts_gui = 2000;
uint32_t nPts = 0;
float stepSize = 0.005;
bool run = true;
bool gpuDirect_gui = false;
bool gpuDirect = false;

float3* cudaPosBuffer = nullptr;
cudaGraphicsResource* glResource = nullptr;
curandState* randState = nullptr;


void initialize() {
  
  nPts = nPts_gui;
  gpuDirect = gpuDirect_gui;

  if(cudaPosBuffer) {
    freeCUDAData(cudaPosBuffer, randState);
  }
  if(glResource) {
    std::cout << "freeing mapped buff " << glResource << std::endl;
    freeOpenGLMappedBuffer(glResource);
    std::cout << "done " << glResource << std::endl;
  }

  initializeCUDAData(cudaPosBuffer, randState, nPts);

  // Create the initial point cloud
  std::vector<glm::vec3> points(nPts);
  polyscope::registerPointCloud("points", points);
  
  if(gpuDirect) {
    uint32_t buffID = polyscope::getPointCloud("points")->getPositionRenderBuffer()->getNativeBufferID();
    initializeOpenGLMappedBuffer(buffID, glResource);
  }
}
void simulationTick() {
  diffusePositions(cudaPosBuffer, randState, stepSize, nPts);
}

void updatePolyscopeData() {

  if (gpuDirect) {

    // copy without leaving the GPU via CUDA interop
    copyPositionsToGL(cudaPosBuffer, glResource, nPts);
    polyscope::getPointCloud("points")->renderBufferDataExternallyUpdated();
    
  } else {
    // copy the "standard" way via the CPU
    std::vector<std::array<float,3>> newPos = getPositionsCPU(cudaPosBuffer, nPts);

    polyscope::getPointCloud("points")->updatePointPositions(newPos);
  }
}

void callback() {

  //ImGui::PushItemWidth(300);

  ImGui::InputInt("num points", &nPts_gui);
  ImGui::SliderFloat("step size", &stepSize, 0.f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

  if (ImGui::Button("re-initialize")) {
    initialize();
  }
  
  ImGui::Checkbox("Direct GPU copy", &gpuDirect_gui);
  ImGui::Checkbox("run", &run);

  //ImGui::PopItemWidth();

  if(run) {
    simulationTick();
    updatePolyscopeData();
  }
}

int main(int argc, char** argv) {

  // Options
  
  polyscope::options::automaticallyComputeSceneExtents = false;
  polyscope::state::lengthScale = 1.;
  polyscope::state::boundingBox = 
      std::tuple<glm::vec3, glm::vec3>{ {-1., -1., -1.}, {1., 1., 1.} };

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;

  printCUDAInfo();

  // Initialize polyscope
  polyscope::init();

  initialize();

  // Add a few gui elements
  polyscope::state::userCallback = callback;

  // Show the gui
  polyscope::show();

  return 0;
}

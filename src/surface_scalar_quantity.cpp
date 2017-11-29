#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name,
                                             SurfaceMesh* mesh_,
                                             DataType dataType_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), dataType(dataType_) {}

double SurfaceScalarQuantity::mapVal(double x) {
  return geometrycentral::clamp((x - minVal) / (maxVal - minVal), 0.0, 1.0);
}

SurfaceScalarVertexQuantity::SurfaceScalarVertexQuantity(
    std::string name, VertexData<double>& values_, SurfaceMesh* mesh_,
    DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, dataType_)

{

  values = parent->transfer.transfer(values_);

  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
  for (VertexPtr v : parent->mesh->vertices()) {
    minVal = std::min(minVal, values[v]);
    maxVal = std::max(maxVal, values[v]);
  }

  // Hack to do less ugly things when constants are passed in
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }
}

gl::GLProgram* SurfaceScalarVertexQuantity::createProgram() {

  // Create the program to draw this quantity
  gl::GLProgram* program = new gl::GLProgram(&VERTCOLOR_SURFACE_VERT_SHADER,
                                        &VERTCOLOR_SURFACE_FRAG_SHADER,
                                        gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceScalarVertexQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<double> colorval;
  for (FacePtr f : parent->mesh->faces()) {
    // Implicitly triangulate
    double c0, c1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      double c2 = mapVal(values[v]);
      if (iP >= 2) {
        colorval.push_back(c0);
        colorval.push_back(c1);
        colorval.push_back(c2);
      }
      c0 = c1;
      c1 = c2;
      iP++;
    }
  }

  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
}


void SurfaceScalarVertexQuantity::draw() {
}  // nothing to do, drawn by surface mesh program



void SurfaceScalarVertexQuantity::drawUI() {

  bool enabledBefore = enabled;
  if (ImGui::TreeNode(name.c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    ImGui::TreePop();
  }

  // Enforce exclusivity of enabled surface quantities
  if (!enabledBefore && enabled) {
    parent->setActiveSurfaceQuantity(this);
  }
  if(enabledBefore && !enabled) {
    parent->clearActiveSurfaceQuantity();
  }
}

}  // namespace polyscope
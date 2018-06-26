#include "polyscope/surface_distance_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/distance_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceDistanceQuantity::SurfaceDistanceQuantity(std::string name, std::vector<double> distances_, SurfaceMesh* mesh_,
                                                 bool signedDist_)
    : SurfaceQuantityThatDrawsFaces(name, mesh_), distances(std::move(distances_)), signedDist(signedDist_) {

  // Set default colormap
  if (signedDist) {
    iColorMap = 1; // coolwarm
  } else {
    iColorMap = 0; // viridis
  }

  // Build the histogram
  std::vector<double> valsVec;
  std::vector<double> weightsVec;
  for(size_t vInd = 0; vInd < parent->triMesh.nVertices(); vInd++) {
    valsVec.push_back(distances[vInd]);
    weightsVec.push_back(parent->triMesh.vertices[vInd].area());
  }
  hist.updateColormap(gl::quantitativeColormaps[iColorMap]);
  hist.buildHistogram(valsVec, weightsVec);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(valsVec, 1e-5);
  resetVizRange();
}

gl::GLProgram* SurfaceDistanceQuantity::createProgram() {
  // Create the program to draw this quantity
  gl::GLProgram* program =
      new gl::GLProgram(&VERT_DIST_SURFACE_VERT_SHADER, &VERT_DIST_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

void SurfaceDistanceQuantity::draw() {}

// Update range uniforms
void SurfaceDistanceQuantity::setProgramValues(gl::GLProgram* program) {
  program->setUniform("u_rangeLow", vizRangeLow);
  program->setUniform("u_rangeHigh", vizRangeHigh);
  program->setUniform("u_modLen", modLen * state::lengthScale);
}

void SurfaceDistanceQuantity::resetVizRange() {
  if (signedDist) {
    float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
    vizRangeLow = -absRange;
    vizRangeHigh = absRange;
  } else {
    vizRangeLow = 0.0;
    vizRangeHigh = dataRangeHigh;
  }
}

void SurfaceDistanceQuantity::drawUI() {
  bool enabledBefore = enabled;
  std::string signedString = signedDist ? "signed distance" : "distance";
  if (ImGui::TreeNode((name + " (" + signedString + ")").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    { // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColormapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, gl::quantitativeColormapNames,
                   IM_ARRAYSIZE(gl::quantitativeColormapNames));
      ImGui::PopItemWidth();
      if (iColorMap != iColormapBefore) {
        parent->deleteProgram();
        hist.updateColormap(gl::quantitativeColormaps[iColorMap]);
      }
    }

    // == Options popup
    ImGui::SameLine();
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      if (ImGui::MenuItem("Write to file")) writeToFile();
      if (ImGui::MenuItem("Reset colormap range")) resetVizRange();

      ImGui::EndPopup();
    }

    // Modulo stripey width
    ImGui::DragFloat("Stripe Length", &modLen, .001, 0.0001, 1.0, "%.4f", 2.0);

    // Draw the histogram of values
    hist.colormapRangeMin = vizRangeLow;
    hist.colormapRangeMax = vizRangeHigh;
    hist.buildUI();

    // Data range
    // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
    // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
    // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
    // %g, unfortunately.
    {
      if (signedDist) {
        float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
        ImGui::DragFloatRange2("##range_symmetric", &vizRangeLow, &vizRangeHigh, absRange / 100., -absRange, absRange,
                               "Min: %.3e", "Max: %.3e");
      } else {
        ImGui::DragFloatRange2("##range_mag", &vizRangeLow, &vizRangeHigh, vizRangeHigh / 100., 0.0, dataRangeHigh,
                               "Min: %.3e", "Max: %.3e");
      }
    }

    ImGui::TreePop();
  }

  // Enforce exclusivity of enabled surface quantities
  if (!enabledBefore && enabled) {
    parent->setActiveSurfaceQuantity(this);
  }
  if (enabledBefore && !enabled) {
    parent->clearActiveSurfaceQuantity();
  }

} // namespace polyscope


void SurfaceDistanceQuantity::fillColorBuffers(gl::GLProgram* p) {
  std::vector<double> colorval;
  colorval.reserve(3 * parent->triMesh.nFaces());
  for (HalfedgeMesh::Face& face : parent->triMesh.faces) {
    for(size_t i = 0; i < 3; i++) {
      size_t vInd = face.triangleVertices()[i]->index();
      colorval.push_back(distances[vInd]); 
    }
  }


  // Store data in buffers
  p->setAttribute("a_colorval", colorval);
  p->setTextureFromColormap("t_colormap", *gl::quantitativeColormaps[iColorMap]);
}

void SurfaceDistanceQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", distances[vInd]);
  ImGui::NextColumn();
}


void SurfaceDistanceQuantity::writeToFile(std::string filename) {

  throw std::runtime_error("NOT IMPLEMENTED");

  /* TODO

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  cout << "Writing distance function to file " << filename << " in U coordinate of texture map" << endl;

  HalfedgeMesh* mesh = parent->mesh;
  CornerData<Vector2> scalarVal(mesh, Vector2{0.0, 0.0});
  for (CornerPtr c : mesh->corners()) {
    scalarVal[c].x = distances[c.vertex()];
  }

  WavefrontOBJ::write(filename, *parent->geometry, scalarVal);
  */
}


} // namespace polyscope

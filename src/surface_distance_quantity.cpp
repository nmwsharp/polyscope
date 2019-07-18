// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_distance_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/distance_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceDistanceQuantity::SurfaceDistanceQuantity(std::string name, std::vector<double> distances_, SurfaceMesh& mesh_,
                                                 bool signedDist_)
    : SurfaceMeshQuantity(name, mesh_, true), distances(std::move(distances_)), signedDist(signedDist_) {

  // Set default colormap
  if (signedDist) {
    cMap = gl::ColorMapID::COOLWARM;
  } else {
    cMap = gl::ColorMapID::VIRIDIS;
  }

  // Build the histogram
  hist.updateColormap(cMap);
  hist.buildHistogram(distances, parent.vertexAreas);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(distances, 1e-5);
  resetVizRange();
}

void SurfaceDistanceQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  setProgramUniforms(*program);

  program->draw();
}

void SurfaceDistanceQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(new gl::GLProgram(&gl::VERT_DIST_SURFACE_VERT_SHADER, &gl::VERT_DIST_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles));

  // Fill color buffers
  fillColorBuffers(*program);
  parent.fillGeometryBuffers(*program);

  setMaterialForProgram(*program, "wax");
}


// Update range uniforms
void SurfaceDistanceQuantity::setProgramUniforms(gl::GLProgram& program) {
  program.setUniform("u_rangeLow", vizRangeLow);
  program.setUniform("u_rangeHigh", vizRangeHigh);
  program.setUniform("u_modLen", modLen * state::lengthScale);
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

void SurfaceDistanceQuantity::buildCustomUI() {
  ImGui::SameLine();

  if (buildColormapSelector(cMap)) {
    program.reset();
    hist.updateColormap(cMap);
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

} // namespace polyscope


void SurfaceDistanceQuantity::fillColorBuffers(gl::GLProgram& p) {
  std::vector<double> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      size_t vC = face[(j + 1) % D];

      colorval.push_back(distances[vRoot]);
      colorval.push_back(distances[vB]);
      colorval.push_back(distances[vC]);
    }
  }


  // Store data in buffers
  p.setAttribute("a_colorval", colorval);
  p.setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
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


std::string SurfaceDistanceQuantity::niceName() {
  std::string signedString = signedDist ? "signed distance" : "distance";
  return name + " (" + signedString + ")";
}

void SurfaceDistanceQuantity::geometryChanged() { program.reset(); }

} // namespace polyscope

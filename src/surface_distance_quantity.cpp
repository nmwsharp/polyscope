// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_distance_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceDistanceQuantity::SurfaceDistanceQuantity(std::string name, std::vector<double> distances_, SurfaceMesh& mesh_,
                                                 bool signedDist_)
    : SurfaceMeshQuantity(name, mesh_, true), distances(std::move(distances_)), signedDist(signedDist_),
      stripeSize(uniquePrefix() + name + "#stripeSize", relativeValue(0.02)),
      cMap(uniquePrefix() + name + "#cmap", signedDist ? "coolwarm" : "viridis")


{

  // Build the histogram
  hist.updateColormap(cMap.get());
  hist.buildHistogram(distances, parent.vertexAreas);

  dataRange = robustMinMax(distances, 1e-5);
  resetMapRange();
}

void SurfaceDistanceQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  parent.setStructureUniforms(*program);
  setProgramUniforms(*program);

  program->draw();
}

void SurfaceDistanceQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE", "ISOLINE_STRIPE_VALUECOLOR"}));

  // Fill color buffers
  fillColorBuffers(*program);
  parent.fillGeometryBuffers(*program);

  render::engine->setMaterial(*program, parent.getMaterial());
}


// Update range uniforms
void SurfaceDistanceQuantity::setProgramUniforms(render::ShaderProgram& program) {
  program.setUniform("u_rangeLow", vizRange.first);
  program.setUniform("u_rangeHigh", vizRange.second);
  program.setUniform("u_modLen", getStripeSize());
}

SurfaceDistanceQuantity* SurfaceDistanceQuantity::resetMapRange() {
  if (signedDist) {
    double absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
    vizRange = std::make_pair(-absRange, absRange);
  } else {
    vizRange = std::make_pair(0., dataRange.second);
  }
  requestRedraw();
  return this;
}

void SurfaceDistanceQuantity::buildCustomUI() {
  ImGui::SameLine();

  if (render::buildColormapSelector(cMap.get())) {
    program.reset();
    setColorMap(getColorMap());
  }

  // == Options popup
  ImGui::SameLine();
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    if (ImGui::MenuItem("Reset colormap range")) resetMapRange();
    ImGui::EndPopup();
  }

  // Modulo stripey width
  if (ImGui::DragFloat("Stripe size", stripeSize.get().getValuePtr(), .001, 0.0001, 1.0, "%.4f", 2.0)) {
    stripeSize.manuallyChanged();
    requestRedraw();
  }

  // Draw the histogram of values
  hist.colormapRange = vizRange;
  hist.buildUI();

  // Data range
  // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    if (signedDist) {
      float absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
      ImGui::DragFloatRange2("##range_symmetric", &vizRange.first, &vizRange.second, absRange / 100., -absRange,
                             absRange, "Min: %.3e", "Max: %.3e");
    } else {
      ImGui::DragFloatRange2("##range_mag", &vizRange.first, &vizRange.second, vizRange.second / 100., 0.0,
                             dataRange.second, "Min: %.3e", "Max: %.3e");
    }
  }
}


void SurfaceDistanceQuantity::fillColorBuffers(render::ShaderProgram& p) {
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
  p.setAttribute("a_value", colorval);
  p.setTextureFromColormap("t_colormap", cMap.get());
}

SurfaceDistanceQuantity* SurfaceDistanceQuantity::setColorMap(std::string name) {
  cMap = name;
  hist.updateColormap(cMap.get());
  requestRedraw();
  return this;
}
std::string SurfaceDistanceQuantity::getColorMap() { return cMap.get(); }

SurfaceDistanceQuantity* SurfaceDistanceQuantity::setMapRange(std::pair<double, double> val) {
  vizRange = val;
  requestRedraw();
  return this;
}
std::pair<double, double> SurfaceDistanceQuantity::getMapRange() { return vizRange; }
SurfaceDistanceQuantity* SurfaceDistanceQuantity::setStripeSize(double size, bool isRelative) {
  stripeSize = ScaledValue<float>(size, isRelative);
  requestRedraw();
  return this;
}
double SurfaceDistanceQuantity::getStripeSize() { return stripeSize.get().asAbsolute(); }


void SurfaceDistanceQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", distances[vInd]);
  ImGui::NextColumn();
}


std::string SurfaceDistanceQuantity::niceName() {
  std::string signedString = signedDist ? "signed distance" : "distance";
  return name + " (" + signedString + ")";
}

void SurfaceDistanceQuantity::refresh() { 
  program.reset(); 
  Quantity::refresh();
}

} // namespace polyscope

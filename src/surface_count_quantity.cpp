// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_count_quantity.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceCountQuantity::SurfaceCountQuantity(std::string name, SurfaceMesh& mesh_, std::string descriptiveType_)
    : SurfaceMeshQuantity(name, mesh_), descriptiveType(descriptiveType_),
      pointRadius(uniquePrefix() + "#pointRadius", relativeValue(0.005)),
      colorMap(uniquePrefix() + "#colorMap", "coolwarm") {}

void SurfaceCountQuantity::initializeLimits() {

  // limits
  sum = 0;
  std::vector<double> allValues;
  for (auto& e : entries) {
    sum += e.second;
    allValues.push_back(e.second);
  }

  robustMinMax(allValues);
  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(allValues);
  vizRangeLow = dataRangeLow;
  vizRangeHigh = dataRangeHigh;
}

void SurfaceCountQuantity::createProgram() {

  program = render::engine->requestShader("RAYCAST_SPHERE", {"SPHERE_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE"});


  // Fill buffers
  std::vector<glm::vec3> pos;
  std::vector<double> value;
  for (auto& e : entries) {
    pos.push_back(e.first);
    value.push_back(e.second);
  }


  // Store data in buffers
  program->setAttribute("a_position", pos);
  program->setAttribute("a_value", value);

  program->setTextureFromColormap("t_colormap", colorMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceCountQuantity::refresh() { 
  program.reset(); 
  Quantity::refresh();
}

void SurfaceCountQuantity::setUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());

  p.setUniform("u_pointRadius", pointRadius.get().asAbsolute());
  p.setUniform("u_rangeLow", vizRangeLow);
  p.setUniform("u_rangeHigh", vizRangeHigh);
}

void SurfaceCountQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  setUniforms(*program);
  parent.setTransformUniforms(*program);

  program->draw();
}

void SurfaceCountQuantity::buildCustomUI() {

  if (render::buildColormapSelector(colorMap.get())) {
    colorMap.manuallyChanged();
    setColorMap(colorMap.get());
    program.reset();
  }
  ImGui::Text("Sum: %d", sum);

  ImGui::DragFloatRange2("Color Range", &vizRangeLow, &vizRangeHigh, (dataRangeHigh - dataRangeLow) / 100.,
                         dataRangeLow, dataRangeHigh, "Min: %.3e", "Max: %.3e");

  if (ImGui::SliderFloat("Radius", pointRadius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    pointRadius.manuallyChanged();
    requestRedraw();
  }
}

std::string SurfaceCountQuantity::niceName() { return name + " (" + descriptiveType + ")"; }


SurfaceCountQuantity* SurfaceCountQuantity::setColorMap(std::string m) {
  colorMap = m;
  requestRedraw();
  return this;
}
std::string SurfaceCountQuantity::getColorMap() { return colorMap.get(); }

SurfaceCountQuantity* SurfaceCountQuantity::setPointRadius(double newVal, bool isRelative) {
  pointRadius = ScaledValue<float>(newVal, isRelative);
  polyscope::requestRedraw();
  return this;
}
double SurfaceCountQuantity::getPointRadius() { return pointRadius.get().asAbsolute(); }


// ========================================================
// ==========           Vertex Count            ==========
// ========================================================

SurfaceVertexCountQuantity::SurfaceVertexCountQuantity(std::string name, std::vector<std::pair<size_t, int>> values_,
                                                       SurfaceMesh& mesh_)
    : SurfaceCountQuantity(name, mesh_, "vertex count")

{

  // Apply permutation if needed
  if (parent.vertexPerm.size() > 0) {

    // Build a temporary map to invert
    std::map<size_t, int> m;
    for (auto& t : values_) {
      m[t.first] = t.second;
    }

    // Invert
    std::vector<std::pair<size_t, int>> newValues;
    for (size_t iP = 0; iP < parent.nVertices(); iP++) {
      if (m.find(parent.vertexPerm[iP]) != m.end()) {
        newValues.emplace_back(iP, m[parent.vertexPerm[iP]]);
      }
    }

    values_ = newValues;
  }


  for (auto& t : values_) {
    values[t.first] = t.second;
    entries.push_back(std::make_pair(parent.vertices[t.first], t.second));
  }

  initializeLimits();
}

void SurfaceVertexCountQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(vInd) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[vInd]);
  }
  ImGui::NextColumn();
}

// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

SurfaceVertexIsolatedScalarQuantity::SurfaceVertexIsolatedScalarQuantity(std::string name,
                                                                         std::vector<std::pair<size_t, double>> values_,
                                                                         SurfaceMesh& mesh_)
    : SurfaceCountQuantity(name, mesh_, "isolated vertex scalar")

{

  // Apply permutation if needed
  if (parent.vertexPerm.size() > 0) {

    // Build a temporary map to invert
    std::map<size_t, double> m;
    for (auto& t : values_) {
      m[t.first] = t.second;
    }

    // Invert
    std::vector<std::pair<size_t, double>> newValues;
    for (size_t iP = 0; iP < parent.nVertices(); iP++) {
      if (m.find(parent.vertexPerm[iP]) != m.end()) {
        newValues.emplace_back(iP, m[parent.vertexPerm[iP]]);
      }
    }

    values_ = newValues;
  }

  for (auto& t : values_) {
    values[t.first] = t.second;
    entries.push_back(std::make_pair(parent.vertices[t.first], t.second));
  }

  initializeLimits();
}

void SurfaceVertexIsolatedScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(vInd) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%g", values[vInd]);
  }
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Count             ==========
// ========================================================

SurfaceFaceCountQuantity::SurfaceFaceCountQuantity(std::string name, std::vector<std::pair<size_t, int>> values_,
                                                   SurfaceMesh& mesh_)
    : SurfaceCountQuantity(name, mesh_, "face count") {

  // Apply permutation if needed
  if (parent.facePerm.size() > 0) {

    // Build a temporary map to invert
    std::map<size_t, int> m;
    for (auto& t : values_) {
      m[t.first] = t.second;
    }

    // Invert
    std::vector<std::pair<size_t, int>> newValues;
    for (size_t iP = 0; iP < parent.nFaces(); iP++) {
      if (m.find(parent.facePerm[iP]) != m.end()) {
        newValues.emplace_back(iP, m[parent.facePerm[iP]]);
      }
    }

    values_ = newValues;
  }

  for (auto& t : values_) {
    values[t.first] = t.second;

    size_t iF = t.first;
    auto& face = parent.faces[iF];
    size_t D = face.size();
    glm::vec3 faceCenter = glm::vec3{0., 0., 0.};
    for (size_t j = 0; j < D; j++) {
      faceCenter += parent.vertices[face[j]];
    }
    faceCenter /= static_cast<double>(D);

    entries.push_back(std::make_pair(faceCenter, t.second));
  }

  initializeLimits();
}

void SurfaceFaceCountQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(fInd) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[fInd]);
  }
  ImGui::NextColumn();
}

} // namespace polyscope

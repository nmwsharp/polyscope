#include "polyscope/surface_count_quantity.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceCountQuantity::SurfaceCountQuantity(std::string name, SurfaceMesh* mesh_, std::string descriptiveType_)
    : SurfaceQuantity(name, mesh_), descriptiveType(descriptiveType_) {}

void SurfaceCountQuantity::prepare() {

  safeDelete(program);
  program = new gl::GLProgram(&SPHERE_VALUE_VERT_SHADER, &SPHERE_VALUE_GEOM_SHADER,
                              &SPHERE_VALUE_FRAG_SHADER, gl::DrawMode::Points);

  // Color limits
  sum = 0;
  std::vector<double> allValues;
  for (auto& e : entries) {
    sum += e.second;
    allValues.push_back(e.second);
  }
  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(allValues);
  vizRangeLow = dataRangeLow;
  vizRangeHigh = dataRangeHigh;

  // Fill buffers
  std::vector<Vector3> pos;
  std::vector<double> value;
  for (auto& e : entries) {
    pos.push_back(e.first);
    value.push_back(e.second);
  }

  // Set the initial colormap to coolwarm
  iColorMap = 1;

  // Store data in buffers
  program->setAttribute("a_position", pos);
  program->setAttribute("a_value", value);

  program->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap]);
}

void SurfaceCountQuantity::setUniforms(gl::GLProgram* p) {

  glm::mat4 viewMat = parent->getModelView();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);


  p->setUniform("u_pointRadius", pointRadius * state::lengthScale);
  p->setUniform("u_baseColor", Vector3{0.0, 0.0, 0.0});

  program->setUniform("u_rangeLow", vizRangeLow);
  program->setUniform("u_rangeHigh", vizRangeHigh);
}

void SurfaceCountQuantity::draw() {
  if (enabled) {
    setUniforms(program);
    program->draw();
  }
}

void SurfaceCountQuantity::drawUI() {
  if (ImGui::TreeNode((name + " (" + descriptiveType + ")").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    { // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColorMapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, gl::allColormapNames, IM_ARRAYSIZE(gl::allColormapNames));
      ImGui::PopItemWidth();
      if (iColorMap != iColorMapBefore) {
        program->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap], true);
      }
    }
    ImGui::Text("Sum: %d", sum);
    
    ImGui::DragFloatRange2("Color Range", &vizRangeLow, &vizRangeHigh, (dataRangeHigh - dataRangeLow) / 100., dataRangeLow,
                             dataRangeHigh, "Min: %.3e", "Max: %.3e");

    ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);
    ImGui::TreePop();
  }
}

// ========================================================
// ==========           Vertex Count            ==========
// ========================================================

SurfaceCountVertexQuantity::SurfaceCountVertexQuantity(std::string name,
                                                       std::vector<std::pair<VertexPtr, int>>& values_,
                                                       SurfaceMesh* mesh_)
    : SurfaceCountQuantity(name, mesh_, "vertex count")

{
  for (auto& x : values_) {
    VertexPtr newV = parent->transfer.vMap[x.first];
    entries.push_back(std::make_pair(parent->geometry->position(newV), x.second));
    values[newV] = x.second;
  }
  prepare();
}

void SurfaceCountVertexQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(v) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[v]);
  }
  ImGui::NextColumn();
}

// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

SurfaceIsolatedScalarVertexQuantity::SurfaceIsolatedScalarVertexQuantity(
    std::string name, std::vector<std::pair<VertexPtr, double>>& values_, SurfaceMesh* mesh_)
    : SurfaceCountQuantity(name, mesh_, "isolated vertex scalar")

{
  for (auto& x : values_) {
    VertexPtr newV = parent->transfer.vMap[x.first];
    entries.push_back(std::make_pair(parent->geometry->position(newV), x.second));
    values[newV] = x.second;
  }
  prepare();
}

void SurfaceIsolatedScalarVertexQuantity::drawUI() {
  if (ImGui::TreeNode((name + " (" + descriptiveType + ")").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    { // Set colormap
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      int iColorMapBefore = iColorMap;
      ImGui::Combo("##colormap", &iColorMap, gl::allColormapNames, IM_ARRAYSIZE(gl::allColormapNames));
      ImGui::PopItemWidth();
      if (iColorMap != iColorMapBefore) {
        program->setTextureFromColormap("t_colormap", *gl::allColormaps[iColorMap], true);
      }
    }
        
    ImGui::DragFloatRange2("Color Range", &vizRangeLow, &vizRangeHigh, (dataRangeHigh - dataRangeLow) / 100., dataRangeLow,
                             dataRangeHigh, "Min: %.3e", "Max: %.3e");

    ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);
    ImGui::TreePop();
  }
}

void SurfaceIsolatedScalarVertexQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(v) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%g", values[v]);
  }
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Count             ==========
// ========================================================

SurfaceCountFaceQuantity::SurfaceCountFaceQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values_,
                                                   SurfaceMesh* mesh_)
    : SurfaceCountQuantity(name, mesh_, "face count") {

  for (auto& x : values_) {
    FacePtr newF = parent->transfer.fMap[x.first];
    entries.push_back(std::make_pair(parent->geometry->barycenter(newF), x.second));
    values[newF] = x.second;
  }
  prepare();
}

void SurfaceCountFaceQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (values.find(f) == values.end()) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[f]);
  }
  ImGui::NextColumn();
}

} // namespace polyscope

#include "polyscope/surface_count_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceCountQuantity::SurfaceCountQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn_)
    : SurfaceQuantity(name, mesh_), definedOn(definedOn_) {}

void SurfaceCountQuantity::prepare() {

  safeDelete(program);
  program = new gl::GLProgram(&PASSTHRU_SPHERE_COLORED_VERT_SHADER, &SPHERE_GEOM_COLORED_BILLBOARD_SHADER, &SHINY_SPHERE_COLORED_FRAG_SHADER,
                              gl::DrawMode::Points);

  // Color limits
  // TODO use AffineRemapper and colormap textures
  double maxIndMag = 0;
  sum = 0;
  for (auto& e : entries) {
    maxIndMag = std::max(maxIndMag, (double)std::abs(e.second));
    sum += e.second;
  }

  const gl::Colormap& cm = gl::CM_COOLWARM;

  // Fill buffers
  std::vector<Vector3> pos;
  std::vector<Vector3> color;
  for (auto& e : entries) {
    pos.push_back(e.first);
    color.push_back(cm.getValue(0.5 * e.second / maxIndMag + 0.5));
  }

  // Store data in buffers
  program->setAttribute("a_position", pos);
  program->setAttribute("a_color", color);
}

void SurfaceCountQuantity::setPointCloudBillboardUniforms(gl::GLProgram* p, bool withLight) {

  glm::mat4 viewMat = view::getCameraViewMatrix();
  p->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  p->setUniform("u_projMatrix", glm::value_ptr(projMat));

  if (withLight) {
    Vector3 eyePos = view::getCameraWorldPosition();
    p->setUniform("u_eye", eyePos);

    p->setUniform("u_lightCenter", state::center);
    p->setUniform("u_lightDist", 2 * state::lengthScale);
  }

  Vector3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);

  p->setUniform("u_camZ", lookDir);
  p->setUniform("u_camUp", upDir);
  p->setUniform("u_camRight", rightDir);

  p->setUniform("u_pointRadius", pointRadius * state::lengthScale);
}

void SurfaceCountQuantity::draw() {
  if (enabled) {
    setPointCloudBillboardUniforms(program, true);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0, -1e3);

    program->draw();
    
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

void SurfaceCountQuantity::drawUI() {
  if (ImGui::TreeNode((name + " (" + definedOn + " count)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::Text("Sum: %d", sum);
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
    : SurfaceCountQuantity(name, mesh_, "vertex")

{
  values = VertexData<int>(parent->mesh, NO_INDEX); 
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
  if(values[v] == NO_INDEX) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[v]);
  }
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Count             ==========
// ========================================================

SurfaceCountFaceQuantity::SurfaceCountFaceQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values_,
                                                   SurfaceMesh* mesh_)
    : SurfaceCountQuantity(name, mesh_, "face") {

  values = FaceData<int>(parent->mesh, NO_INDEX);
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
  if(values[f] == NO_INDEX) {
    ImGui::TextUnformatted("-");
  } else {
    ImGui::Text("%+d", values[f]);
  }
  ImGui::NextColumn();
}

} // namespace polyscope

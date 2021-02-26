#include "polyscope/slice_plane.h"

#include "polyscope/polyscope.h"

namespace polyscope {

// NOTE: Unfortunately, the logic here and in the engine depends on the names constructed from the postfix being
// identical.

SlicePlane* addSlicePlane() {
  size_t nPlanes = state::slicePlanes.size();
  std::string postfix = std::to_string(nPlanes);
  std::string newName = "Scene Slice Plane " + std::to_string(nPlanes);
  render::engine->addSlicePlane(postfix);
  return new SlicePlane(newName, postfix);
}

void buildSlicePlaneGUI() {}

SlicePlane::SlicePlane(std::string name_, std::string postfix_)
    : name(name_), postfix(postfix_), enabled("SlicePlane#" + name + "#enabled", true),
      show("SlicePlane#" + name + "#show", true),
      objectTransform("SlicePlane#" + name + "#object_transform", glm::mat4(1.0)),
      transparency("SlicePlane#" + name + "#transparency", 0.3),
      transformGizmo("SlicePlane#" + name + "#transform_gizmo", objectTransform.get(), &objectTransform)

{
  state::slicePlanes.push_back(this);
  transformGizmo.enabled = true;
  prepare();
}

SlicePlane::~SlicePlane() {
  auto pos = std::find(state::slicePlanes.begin(), state::slicePlanes.end(), this);
  if (pos == state::slicePlanes.end()) return;
  state::slicePlanes.erase(pos);
}

void SlicePlane::prepare() {

  planeProgram = render::engine->requestShader("SLICE_PLANE", {}, render::ShaderReplacementDefaults::Process);

  // Geometry of the plane, using triangles with vertices at infinity
  glm::vec4 cVert{0., 0., 0., 1.};
  glm::vec4 v1{0., 0., 1., 0.};
  glm::vec4 v2{0., 1., 0., 0.};
  glm::vec4 v3{0., 0., -1., 0.};
  glm::vec4 v4{0., -1., 0., 0.};

  // clang-format off
  std::vector<glm::vec4> positions = {
    cVert, v2, v1,
    cVert, v3, v2,
    cVert, v4, v3,
    cVert, v1, v4
  };
  // clang-format on

  planeProgram->setAttribute("a_position", positions);
}

void SlicePlane::draw() {
  if (!enabled.get() || !show.get()) return;

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  planeProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));
  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  planeProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  planeProgram->setUniform("u_objectMatrix", glm::value_ptr(objectTransform.get()));
  planeProgram->setUniform("u_lengthScale", state::lengthScale);
  planeProgram->setUniform("u_transparency", transparency.get());

  // glm::vec3 center{objectTransform.get()[3][0], objectTransform.get()[3][1], objectTransform.get()[3][2]};
  // planeProgram->setUniform("u_center", center);

  render::engine->setDepthMode(DepthMode::Less);
  render::engine->setBackfaceCull(false);
  render::engine->applyTransparencySettings();
  planeProgram->draw();
}

void SlicePlane::buildGUI() {
  ImGui::PushID(name.c_str());

  ImGui::TextUnformatted(name.c_str());
  if (ImGui::Checkbox("enabled", &enabled.get())) enabled.manuallyChanged();
  ImGui::SameLine();
  if (ImGui::Checkbox("draw plane", &show.get())) {
    show.manuallyChanged();
    transformGizmo.enabled = show.get();
  }

  ImGui::PopID();
}

void SlicePlane::setSceneObjectUniforms(render::ShaderProgram& p) {
  p.setUniform("u_slicePlaneCenter_" + postfix, getCenter());
  p.setUniform("u_slicePlaneNormal_" + postfix, getNormal());
}

glm::vec3 SlicePlane::getCenter() {
  if (enabled.get()) {
    glm::vec3 center{objectTransform.get()[3][0], objectTransform.get()[3][1], objectTransform.get()[3][2]};
    return center;
  } else {
    // Put it really far away so tests always pass
    return glm::vec3{std::numeric_limits<float>::infinity(), 0., 0.};
  }
}

glm::vec3 SlicePlane::getNormal() {
  if (enabled.get()) {
    glm::vec3 normal{objectTransform.get()[0][0], objectTransform.get()[0][1], objectTransform.get()[0][2]};
    normal = glm::normalize(normal);
    return normal;
  } else {
    // Matched with center so tests always pass when disabled
    return glm::vec3{-1., 0., 0.};
  }
}

} // namespace polyscope

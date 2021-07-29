#include "polyscope/slice_plane.h"

#include "polyscope/polyscope.h"

namespace polyscope {

// NOTE: Unfortunately, the logic here and in the engine depends on the names constructed from the postfix being
// identical.

namespace {
// storage for slice planes "owned" by the scene itself
// note: it would be nice for these to be unique_ptr<>, but unfortunately we fall in to a bad design trap---since
// destruction order is essentially arbitrary, these might get destructed after other Polyscope data, causing faults
// when the destructor executes. The lame solution is to just store as raw pointers, it only makes a difference at
// program exit.
std::vector<SlicePlane*> sceneSlicePlanes;

} // namespace

// Storage for global options
bool openSlicePlaneMenu = false;

SlicePlane* addSceneSlicePlane(bool initiallyVisible) {
  size_t nPlanes = sceneSlicePlanes.size();
  std::string newName = "Scene Slice Plane " + std::to_string(nPlanes);
  sceneSlicePlanes.emplace_back(new SlicePlane(newName));
  if(!initiallyVisible) {
    sceneSlicePlanes.back()->setDrawPlane(false);
    sceneSlicePlanes.back()->setDrawWidget(false);
  }
  return sceneSlicePlanes.back();
}

void removeLastSceneSlicePlane() {
  if (sceneSlicePlanes.empty()) return;
  delete sceneSlicePlanes.back();
  sceneSlicePlanes.pop_back();
}

void buildSlicePlaneGUI() {


  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (openSlicePlaneMenu) {
    ImGui::SetNextTreeNodeOpen(true);
    openSlicePlaneMenu = false;
  }
  if (ImGui::TreeNode("Slice Planes")) {
    if (ImGui::Button("Add plane")) {
      addSceneSlicePlane(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove plane")) {
      removeLastSceneSlicePlane();
    }
    for (SlicePlane* s : state::slicePlanes) {
      s->buildGUI();
    }
    ImGui::TreePop();
  }
}

SlicePlane::SlicePlane(std::string name_)
    : name(name_), postfix(std::to_string(state::slicePlanes.size())), active("SlicePlane#" + name + "#active", true),
      drawPlane("SlicePlane#" + name + "#drawPlane", true), drawWidget("SlicePlane#" + name + "#drawWidget", true),
      objectTransform("SlicePlane#" + name + "#object_transform", glm::mat4(1.0)),
      color("SlicePlane#" + name + "#color", getNextUniqueColor()),
      transparency("SlicePlane#" + name + "#transparency", 0.5),
      transformGizmo("SlicePlane#" + name + "#transform_gizmo", objectTransform.get(), &objectTransform) {
  state::slicePlanes.push_back(this);
  render::engine->addSlicePlane(postfix);
  transformGizmo.enabled = true;
  prepare();
}

SlicePlane::~SlicePlane() {
  render::engine->removeSlicePlane(postfix);
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
  if (!drawPlane.get() || !active.get()) return;

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  planeProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));
  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  planeProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  planeProgram->setUniform("u_objectMatrix", glm::value_ptr(objectTransform.get()));
  planeProgram->setUniform("u_lengthScale", state::lengthScale);
  planeProgram->setUniform("u_color", color.get());
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

  if (ImGui::Checkbox(name.c_str(), &active.get())) {
    setActive(getActive());
  }
  ImGui::Indent(16.);
  if (ImGui::Checkbox("draw plane", &drawPlane.get())) {
    setDrawPlane(getDrawPlane());
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("draw widget", &drawWidget.get())) {
    setDrawWidget(getDrawWidget());
  }
  ImGui::Unindent(16.);

  ImGui::PopID();
}

void SlicePlane::setSceneObjectUniforms(render::ShaderProgram& p, bool alwaysPass) {
  glm::vec3 normal, center;

  if (alwaysPass) {
    normal = glm::vec3{-1., 0., 0.};
    center = glm::vec3{std::numeric_limits<float>::infinity(), 0., 0.};
  } else {
    glm::mat4 viewMat = view::getCameraViewMatrix();
    normal = glm::vec3(viewMat * glm::vec4(getNormal(), 0.));
    center = glm::vec3(viewMat * glm::vec4(getCenter(), 1.));
  }

  p.setUniform("u_slicePlaneNormal_" + postfix, normal);
  p.setUniform("u_slicePlaneCenter_" + postfix, center);
}

glm::vec3 SlicePlane::getCenter() {
  if (active.get()) {
    glm::vec3 center{objectTransform.get()[3][0], objectTransform.get()[3][1], objectTransform.get()[3][2]};
    return center;
  } else {
    // Put it really far away so tests always pass
    return glm::vec3{std::numeric_limits<float>::infinity(), 0., 0.};
  }
}

glm::vec3 SlicePlane::getNormal() {
  if (active.get()) {
    glm::vec3 normal{objectTransform.get()[0][0], objectTransform.get()[0][1], objectTransform.get()[0][2]};
    normal = glm::normalize(normal);
    return normal;
  } else {
    // Matched with center so tests always pass when disabled
    return glm::vec3{-1., 0., 0.};
  }
}

void SlicePlane::updateWidgetEnabled() {
  bool enabled = getActive() && getDrawWidget();
  transformGizmo.enabled = enabled;
}
  
void SlicePlane::setPose(glm::vec3 planePosition, glm::vec3 planeNormal) {

  // Choose the other axes to be most similar to the current ones, which will make animations look smoother
  // if the grid is shown
  glm::vec3 currBasisX{objectTransform.get()[1][0], objectTransform.get()[1][1], objectTransform.get()[1][2]};
  glm::vec3 currBasisY{objectTransform.get()[2][0], objectTransform.get()[2][1], objectTransform.get()[2][2]};

  glm::vec3 normal = glm::normalize(planeNormal);
  glm::vec3 basisX = currBasisX - normal * glm::dot(normal, currBasisX);
  if(glm::length(basisX) < 0.01) basisX = currBasisY - normal * glm::dot(normal, currBasisY);
  basisX = glm::normalize(basisX);
  glm::vec3 basisY = glm::cross(normal, basisX);

  // Build the rotation component
  glm::mat4x4 newTransform = glm::mat4x4(1.0);
  for(int i = 0; i < 3; i++) newTransform[0][i] = normal[i];
  for(int i = 0; i < 3; i++) newTransform[1][i] = basisX[i];
  for(int i = 0; i < 3; i++) newTransform[2][i] = basisY[i];
  
  // Build the translation component
  for(int i = 0; i < 3; i++) newTransform[3][i] = planePosition[i];

  objectTransform = newTransform;
  polyscope::requestRedraw();
}

bool SlicePlane::getActive() { return active.get(); }
void SlicePlane::setActive(bool newVal) {
  active = newVal;
  updateWidgetEnabled();
  polyscope::requestRedraw();
}

bool SlicePlane::getDrawPlane() { return drawPlane.get(); }
void SlicePlane::setDrawPlane(bool newVal) {
  drawPlane = newVal;
  polyscope::requestRedraw();
}

bool SlicePlane::getDrawWidget() { return drawWidget.get(); }
void SlicePlane::setDrawWidget(bool newVal) {
  drawWidget = newVal;
  updateWidgetEnabled();
  polyscope::requestRedraw();
}

glm::mat4 SlicePlane::getTransform() { return objectTransform.get(); }
void SlicePlane::setTransform(glm::mat4 newTransform) {
  objectTransform = newTransform;
  polyscope::requestRedraw();
}

} // namespace polyscope

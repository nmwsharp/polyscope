#include "polyscope/surface_input_curve_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/cylinder_shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

using std::cout;
using std::endl;

SurfaceInputCurveQuantity::SurfaceInputCurveQuantity(std::string name, SurfaceMesh* mesh_)
    : SurfaceQuantity(name, mesh_), curve(parent->geometry) {
  curveColor = mesh_->colorManager.getNextSubColor(name);

  // Create the program
  program = new gl::GLProgram(&PASSTHRU_CYLINDER_VERT_SHADER, &CYLINDER_GEOM_SHADER, &SHINY_CYLINDER_FRAG_SHADER,
                              gl::DrawMode::Points);
}

SurfaceInputCurveQuantity::~SurfaceInputCurveQuantity() { safeDelete(program); }


void SurfaceInputCurveQuantity::draw() {

  if (!enabled) return;

  if (bufferStale) fillBuffers();

  // Set uniforms
  glm::mat4 viewMat = parent->getModelView();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_radius", radiusParam * state::lengthScale);
  program->setUniform("u_color", curveColor);


  program->draw();
}

void SurfaceInputCurveQuantity::fillBuffers() {

  std::vector<Vector3> pTail, pTip;

  for (CurveSegment& c : curve.getCurveSegments()) {
    pTail.push_back(c.startPosition);
    pTip.push_back(c.endPosition);
  }

  program->setAttribute("a_position_tail", pTail);
  program->setAttribute("a_position_tip", pTip);

  bufferStale = false;
}

void SurfaceInputCurveQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (surface curve)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Color", (float*)&curveColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SliderFloat("Radius", &radiusParam, 0.0, .1, "%.5f", 3.);

    if (allowEditingFromDefaultUI) {
      if (ImGui::Button("Edit")) {
        userEdit();
      }
    }

    ImGui::TreePop();
  }
}


void SurfaceInputCurveQuantity::userEdit() {

  // Make sure we can see what we're editing
  enabled = true;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();
  bool oldAlwaysPick = pick::alwaysEvaluatePick;
  pick::alwaysEvaluatePick = true;

  // Register the callback which creates the UI and does the hard work
  focusedPopupUI = std::bind(&SurfaceInputCurveQuantity::userEditCallback, this);

  // Re-enter main loop
  while (focusedPopupUI) {
    mainLoopIteration();
  }

  // Restore the old context
  pick::alwaysEvaluatePick = oldAlwaysPick;
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);
}

void SurfaceInputCurveQuantity::userEditCallback() {

  static bool showWindow = true;
  ImGui::Begin("Edit Curve", &showWindow);

  // Process mouse selection if the ctrl key is held, the mouse is pressed, and the mouse isn't on the ImGui window
  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl && !io.WantCaptureMouse && ImGui::IsMouseDown(0)) {

    // Check what face, if any, was clicked
    FacePtr fClick;
    Vector3 bCoord;
    parent->getPickedFacePoint(fClick, bCoord);

    if (fClick != FacePtr()) {
      curve.tryExtendBack(fClick, bCoord);
      bufferStale = true;
    }
  }


  // Shrink the selection
  if (ImGui::Button("Remove last")) {
    curve.removeLastEndpoint();
    bufferStale = true;
  }


  // Clear
  if (ImGui::Button("Clear")) {
    curve.clearCurve();
    bufferStale = true;
  }


  // Stop editing
  // (style makes yellow button)
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1. / 7.0f, 0.6f, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1. / 7.0f, 0.7f, 0.7f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1. / 7.0f, 0.8f, 0.8f));
  if (ImGui::Button("Done")) {
    focusedPopupUI = nullptr;
  }
  ImGui::PopStyleColor(3);

  ImGui::End();
}


MeshEmbeddedCurve SurfaceInputCurveQuantity::getCurve() {
  return curve.copy(parent->transfer, parent->originalGeometry);
}

} // namespace polyscope

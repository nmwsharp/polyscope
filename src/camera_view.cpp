// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/camera_view.h"

#include "polyscope/file_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include "imgui.h"
#include "polyscope/view.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string CameraView::structureTypeName = "Camera View";

// Constructor
CameraView::CameraView(std::string name, const CameraParameters& params_)
    : QuantityStructure<CameraView>(name, structureTypeName), params(params_),
      widgetFocalLength(uniquePrefix() + "#widgetFocalLength", relativeValue(0.05)),
      widgetThickness(uniquePrefix() + "#widgetThickness", 0.02),
      widgetColor(uniquePrefix() + "#widgetColor", glm::vec3{0., 0., 0.}) {

  updateObjectSpaceBounds();
}


void CameraView::draw() {
  if (!isEnabled()) {
    return;
  }

  // Ensure we have prepared buffers
  if (nodeProgram == nullptr || edgeProgram == nullptr) {
    prepare();
  }


  // The camera frame geometry attributes depend on the scene length scale. If the length scale has changed, regenerate
  // those attributes. (It would be better if we could implement the frame geometry in uniforms only, so we don't have
  // to do this)
  if (preparedLengthScale != state::lengthScale) {
    fillCameraWidgetGeometry(nodeProgram.get(), edgeProgram.get(), nullptr);
  }

  // Set program uniforms
  setStructureUniforms(*nodeProgram);
  setStructureUniforms(*edgeProgram);
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  nodeProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  nodeProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  nodeProgram->setUniform("u_pointRadius", getWidgetFocalLength() * getWidgetThickness());
  nodeProgram->setUniform("u_baseColor", widgetColor.get());


  edgeProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  edgeProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  edgeProgram->setUniform("u_radius", getWidgetFocalLength() * getWidgetThickness());
  edgeProgram->setUniform("u_baseColor", widgetColor.get());

  // Draw the camera view wireframe
  nodeProgram->draw();
  edgeProgram->draw();

  render::engine->applyTransparencySettings();

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void CameraView::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}

void CameraView::drawPick() {
  if (!isEnabled()) {
    return;
  }

  // Ensure we have prepared buffers
  if (pickFrameProgram == nullptr) {
    preparePick();
  }

  // The camera frame geometry attributes depend on the scene length scale. If the length scale has changed, regenerate
  // those attributes. (It would be better if we could implement the frame geometry in uniforms only, so we don't have
  // to do this)
  if (pickPreparedLengthScale != state::lengthScale) {
    fillCameraWidgetGeometry(nullptr, nullptr, pickFrameProgram.get());
  }

  // Set uniforms
  setStructureUniforms(*pickFrameProgram);

  pickFrameProgram->draw();
}

void CameraView::prepare() {

  {
    std::vector<std::string> rules = addStructureRules({"SHADE_BASECOLOR"});
    if (wantsCullPosition()) rules.push_back("SPHERE_CULLPOS_FROM_CENTER");
    nodeProgram = render::engine->requestShader("RAYCAST_SPHERE", rules);
    render::engine->setMaterial(*nodeProgram, "flat");
  }

  {
    std::vector<std::string> rules = addStructureRules({"SHADE_BASECOLOR"});
    if (wantsCullPosition()) rules.push_back("CYLINDER_CULLPOS_FROM_MID");
    edgeProgram = render::engine->requestShader("RAYCAST_CYLINDER", rules);
    render::engine->setMaterial(*edgeProgram, "flat");
  }

  // Fill out the geometry data for the programs
  fillCameraWidgetGeometry(nodeProgram.get(), edgeProgram.get(), nullptr);
}


void CameraView::preparePick() {

  // Request pick indices if we don't already have them
  if (pickStart == INVALID_IND) {
    size_t pickCount = 1;
    pickStart = pick::requestPickBufferRange(this, pickCount);
    pickColor = pick::indToVec(pickStart);
  }

  // Create a new pick program
  std::vector<std::string> rules = addStructureRules({"MESH_PROPAGATE_PICK_SIMPLE"});
  if (wantsCullPosition()) rules.push_back("MESH_PROPAGATE_CULLPOS");
  pickFrameProgram = render::engine->requestShader("MESH", rules, render::ShaderReplacementDefaults::Pick);

  // Store data in buffers
  fillCameraWidgetGeometry(nullptr, nullptr, pickFrameProgram.get());
}


void CameraView::fillCameraWidgetGeometry(render::ShaderProgram* nodeProgram, render::ShaderProgram* edgeProgram,
                                          render::ShaderProgram* pickFrameProgram) {

  // NOTE: this coullllld be done with uniforms, so we don't have to ever edit the geometry at all.
  // FOV slightly tricky though.


  // Camera frame geometry
  // NOTE: some of this is duplicated in getFrameBillboardGeometry()
  glm::vec3 root = params.getPosition();
  glm::vec3 lookDir, upDir, rightDir;
  std::tie(lookDir, upDir, rightDir) = params.getCameraFrame();

  glm::vec3 frameCenter = root + lookDir * widgetFocalLength.get().asAbsolute();
  float halfHeight = static_cast<float>(widgetFocalLength.get().asAbsolute() *
                                        std::tan(glm::radians(params.getFoVVerticalDegrees()) / 2.));
  glm::vec3 frameUp = upDir * halfHeight;
  float halfWidth = params.getAspectRatioWidthOverHeight() * halfHeight;
  glm::vec3 frameLeft = -glm::cross(lookDir, upDir) * halfWidth;

  glm::vec3 frameUpperLeft = frameCenter + frameUp + frameLeft;
  glm::vec3 frameUpperRight = frameCenter + frameUp - frameLeft;
  glm::vec3 frameLowerLeft = frameCenter - frameUp + frameLeft;
  glm::vec3 frameLowerRight = frameCenter - frameUp - frameLeft;
  glm::vec3 triangleLeft = frameCenter + 1.2f * frameUp + 0.7f * frameLeft;
  glm::vec3 triangleRight = frameCenter + 1.2f * frameUp - 0.7f * frameLeft;
  glm::vec3 triangleTop = frameCenter + 2.f * frameUp;

  if (nodeProgram) {
    std::vector<glm::vec3> allPos{root,        frameUpperLeft, frameUpperRight, frameLowerLeft, frameLowerRight,
                                  triangleTop, triangleLeft,   triangleRight};
    nodeProgram->setAttribute("a_position", allPos);
    preparedLengthScale = state::lengthScale;
  }

  if (edgeProgram) {
    // Fill edges
    std::vector<glm::vec3> posTail(11);
    std::vector<glm::vec3> posTip(11);
    auto addEdge = [&](glm::vec3 a, glm::vec3 b) {
      posTail.push_back(a);
      posTip.push_back(b);
    };

    addEdge(root, frameUpperLeft);
    addEdge(root, frameUpperRight);
    addEdge(root, frameLowerLeft);
    addEdge(root, frameLowerRight);
    addEdge(frameUpperLeft, frameUpperRight);
    addEdge(frameUpperRight, frameLowerRight);
    addEdge(frameLowerRight, frameLowerLeft);
    addEdge(frameLowerLeft, frameUpperLeft);
    addEdge(triangleLeft, triangleRight);
    addEdge(triangleRight, triangleTop);
    addEdge(triangleTop, triangleLeft);

    edgeProgram->setAttribute("a_position_tail", posTail);
    edgeProgram->setAttribute("a_position_tip", posTip);
  }

  if (pickFrameProgram) {

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> bcoord;
    std::vector<glm::vec3> cullPos;

    auto addPolygon = [&](std::vector<glm::vec3> vertices) {
      size_t D = vertices.size();

      // implicitly triangulate from root
      glm::vec3 pRoot = vertices[0];
      for (size_t j = 1; (j + 1) < D; j++) {
        glm::vec3 pB = vertices[j];
        glm::vec3 pC = vertices[(j + 1) % D];
        glm::vec3 faceN = glm::cross(pB - pRoot, pC - pRoot);

        // Vertex positions
        positions.push_back(pRoot);
        positions.push_back(pB);
        positions.push_back(pC);

        // Face normals
        normals.push_back(faceN);
        normals.push_back(faceN);
        normals.push_back(faceN);

        // Bary coords
        bcoord.push_back(glm::vec3{1., 0., 0.});
        bcoord.push_back(glm::vec3{0., 1., 0.});
        bcoord.push_back(glm::vec3{0., 0., 1.});

        // Cull position
        cullPos.push_back(root);
        cullPos.push_back(root);
        cullPos.push_back(root);
      }

      pickPreparedLengthScale = state::lengthScale;
    };

    addPolygon({root, frameUpperRight, frameUpperLeft});
    addPolygon({root, frameLowerRight, frameUpperRight});
    addPolygon({root, frameLowerLeft, frameLowerRight});
    addPolygon({root, frameUpperLeft, frameLowerLeft});
    addPolygon({frameUpperLeft, frameUpperRight, frameLowerRight, frameLowerLeft});
    addPolygon({triangleTop, triangleRight, triangleLeft});

    pickFrameProgram->setAttribute("a_vertexPositions", positions);
    // pickFrameProgram->setAttribute("a_vertexNormals", normals); // unused
    pickFrameProgram->setAttribute("a_barycoord", bcoord);

    size_t nFaces = 7;
    std::vector<glm::vec3> faceColor(3 * nFaces, pickColor);
    std::vector<std::array<glm::vec3, 3>> tripleColors(3 * nFaces,
                                                       std::array<glm::vec3, 3>{pickColor, pickColor, pickColor});
    pickFrameProgram->setAttribute<glm::vec3, 3>("a_vertexColors", tripleColors);
    pickFrameProgram->setAttribute("a_faceColor", faceColor);
    if (wantsCullPosition()) {
      pickFrameProgram->setAttribute("a_cullPos", cullPos);
    }
  }
}

void CameraView::updateCameraParameters(const CameraParameters& newParams) {
  params = newParams;
  geometryChanged();
}

void CameraView::geometryChanged() {
  // if the programs are populated, repopulate them
  if (nodeProgram) {
    fillCameraWidgetGeometry(nodeProgram.get(), edgeProgram.get(), nullptr);
  }
  if (pickFrameProgram) {
    fillCameraWidgetGeometry(nullptr, nullptr, pickFrameProgram.get());
  }

  requestRedraw();
  QuantityStructure<CameraView>::refresh();
}

void CameraView::buildPickUI(size_t localPickID) {
  ImGui::Text("center: %s", to_string(params.getPosition()).c_str());
  ImGui::Text("look dir: %s", to_string(params.getLookDir()).c_str());
  ImGui::Text("up dir: %s", to_string(params.getUpDir()).c_str());
  ImGui::Text("FoV (vert): %0.1f deg   aspect ratio: %.2f", params.getFoVVerticalDegrees(),
              params.getAspectRatioWidthOverHeight());
  if (ImGui::Button("fly to")) {
    setViewToThisCamera(true);
  }


  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildPickUI(localPickID);
  }

  ImGui::Indent(-20.);
}

void CameraView::buildCustomUI() {


  ImGui::SameLine();

  { // colors
    if (ImGui::ColorEdit3("Color", &widgetColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setWidgetColor(widgetColor.get());
  }

  if (ImGui::Button("fly to")) {
    setViewToThisCamera(true);
  }
  ImGui::SameLine();
  ImGui::Text("FoV: %0.1f deg   aspect: %.2f", params.getFoVVerticalDegrees(), params.getAspectRatioWidthOverHeight());
}

void CameraView::buildCustomOptionsUI() {

  ImGui::PushItemWidth(150);

  if (widgetFocalLengthUpper == -777) widgetFocalLengthUpper = 2. * (*widgetFocalLength.get().getValuePtr());
  if (ImGui::SliderFloat("widget focal length", widgetFocalLength.get().getValuePtr(), 0, widgetFocalLengthUpper,
                         "%.5f")) {
    widgetFocalLength.manuallyChanged();
    geometryChanged();
    requestRedraw();
  }
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    // the upper bound for the slider is dynamically adjust to be a bit bigger than the lower bound, but only does
    // so on release of the widget (so it doesn't scaleo off to infinity)
    widgetFocalLengthUpper = std::fmax(2. * (*widgetFocalLength.get().getValuePtr()), 0.0001);
  }

  if (ImGui::SliderFloat("widget thickness", &widgetThickness.get(), 0, 0.2, "%.5f")) {
    widgetThickness.manuallyChanged();
    requestRedraw();
  }

  ImGui::PopItemWidth();
}

void CameraView::updateObjectSpaceBounds() {

  // bounding box is just the camera root location
  glm::vec3 cameraPos = params.getPosition();
  objectSpaceBoundingBox = std::make_tuple(cameraPos, cameraPos);

  // no is there an obvious length scale...?
  // we don't use it for much currently
  objectSpaceLengthScale = 0.;
}


std::string CameraView::typeName() { return structureTypeName; }


void CameraView::refresh() {
  nodeProgram.reset();
  edgeProgram.reset();
  pickFrameProgram.reset();
  QuantityStructure<CameraView>::refresh(); // call base class version, which refreshes quantities
}

void CameraView::setViewToThisCamera(bool withFlight) {

  // Adjust the params to push the view forward by eps so it doesn't clip into the frame
  glm::vec3 look, up, right;
  std::tie(look, up, right) = params.getCameraFrame();
  glm::vec3 root = params.getPosition();
  root += look * getWidgetFocalLength() * 0.01f;

  CameraParameters adjParams(params.intrinsics, CameraExtrinsics::fromVectors(root, look, up));

  if (withFlight) {
    view::startFlightTo(adjParams);
  } else {
    view::setViewToCamera(adjParams);
  }
}

// === Quantities


// === Setters and getters

CameraParameters CameraView::getCameraParameters() const { return params; }

CameraView* CameraView::setWidgetFocalLength(float newVal, bool isRelative) {
  widgetFocalLength = ScaledValue<float>(newVal, isRelative);
  geometryChanged();
  polyscope::requestRedraw();
  return this;
}
float CameraView::getWidgetFocalLength() { return widgetFocalLength.get().asAbsolute(); }

CameraView* CameraView::setWidgetThickness(float newVal) {
  widgetThickness = newVal;
  polyscope::requestRedraw();
  return this;
}
float CameraView::getWidgetThickness() { return widgetThickness.get(); }

CameraView* CameraView::setWidgetColor(glm::vec3 val) {
  widgetColor = val;
  requestRedraw();
  return this;
}
glm::vec3 CameraView::getWidgetColor() { return widgetColor.get(); }


std::tuple<glm::vec3, glm::vec3, glm::vec3> CameraView::getFrameBillboardGeometry() {

  // NOTE: duplicated from fillCameraWidgetGeometry()
  glm::vec3 root = params.getPosition();
  glm::vec3 lookDir, upDir, rightDir;
  std::tie(lookDir, upDir, rightDir) = params.getCameraFrame();
  glm::vec3 frameCenter = root + lookDir * widgetFocalLength.get().asAbsolute();
  float halfHeight = static_cast<float>(widgetFocalLength.get().asAbsolute() *
                                        std::tan(glm::radians(params.getFoVVerticalDegrees()) / 2.));
  glm::vec3 frameUp = upDir * halfHeight;
  float halfWidth = params.getAspectRatioWidthOverHeight() * halfHeight;
  glm::vec3 frameRight = glm::cross(lookDir, upDir) * halfWidth;

  return std::tuple<glm::vec3, glm::vec3, glm::vec3>(frameCenter, frameUp, frameRight);
}

} // namespace polyscope

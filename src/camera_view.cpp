// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/camera_view.h"

#include "polyscope/file_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string CameraView::structureTypeName = "Camera View";

// Constructor
CameraView::CameraView(std::string name, const CameraParameters& params_)
    : QuantityStructure<CameraView>(name, structureTypeName), params(std::move(params_)),
      displayFocalLength(uniquePrefix() + "#displayFocalLength", relativeValue(0.1)),
      displayThickness(uniquePrefix() + "#displayThickness", 0.02),
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

  // Set program uniforms
  setStructureUniforms(*nodeProgram);
  setStructureUniforms(*edgeProgram);
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  nodeProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  nodeProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  nodeProgram->setUniform("u_pointRadius", getDisplayFocalLength() * getDisplayThickness());
  nodeProgram->setUniform("u_baseColor", widgetColor.get());


  edgeProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  edgeProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  edgeProgram->setUniform("u_radius", getDisplayFocalLength() * getDisplayThickness());
  edgeProgram->setUniform("u_baseColor", widgetColor.get());

  /*
  frameProgram->setUniform("u_baseColor", widgetColor.get());
  frameProgram->setUniform("u_edgeWidth", displayThickness.get());
  frameProgram->setUniform("u_edgeColor", widgetColor.get());
  */


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

  // Set uniforms
  setStructureUniforms(*pickFrameProgram);

  pickFrameProgram->draw();
}

void CameraView::prepare() {

  {
    nodeProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SHADE_BASECOLOR"});
    render::engine->setMaterial(*nodeProgram, "flat");
  }

  {
    // frameProgram = render::engine->requestShader("MESH", {"SHADE_BASECOLOR", "MESH_WIREFRAME",
    // "MESH_WIREFRAME_ONLY"});
    edgeProgram = render::engine->requestShader("RAYCAST_CYLINDER", {"SHADE_BASECOLOR"});
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
  pickFrameProgram =
      render::engine->requestShader("MESH", {"MESH_PROPAGATE_PICK"}, render::ShaderReplacementDefaults::Pick);


  // Store data in buffers
  fillCameraWidgetGeometry(nullptr, nullptr, pickFrameProgram.get());
}


void CameraView::fillCameraWidgetGeometry(render::ShaderProgram* nodeProgram, render::ShaderProgram* edgeProgram,
                                          render::ShaderProgram* pickFrameProgram) {

  // NOTE: this coullllld be done with uniforms, so we don't have to ever edit the geometry at all.
  // FOV slightly tricky though.

  // Camera frame geometry
  glm::vec3 root = params.getPosition();
  glm::vec3 lookDir, upDir, rightDir;
  std::tie(lookDir, upDir, rightDir) = params.getCameraFrame();

  glm::vec3 frameCenter = root + lookDir * displayFocalLength.get().asAbsolute();
  float halfHeight =
      static_cast<float>(displayFocalLength.get().asAbsolute() * std::tan(glm::radians(params.fov) / 2.));
  glm::vec3 frameUp = upDir * halfHeight;
  float halfWidth = params.aspectRatio * halfHeight;
  glm::vec3 frameLeft = glm::cross(lookDir, upDir) * halfWidth;

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
    // std::vector<glm::vec3> edgeReal;

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

        // Set which edges are real
        // glm::vec3 edgeRealV{0., 1., 0.};
        // if (j == 1) {
        // edgeRealV.x = 1.;
        //}
        // if (j + 2 == D) {
        // edgeRealV.z = 1.;
        //}
        // edgeReal.push_back(edgeRealV);
        // edgeReal.push_back(edgeRealV);
        // edgeReal.push_back(edgeRealV);
      }
    };

    addPolygon({root, frameUpperRight, frameUpperLeft});
    addPolygon({root, frameLowerRight, frameUpperRight});
    addPolygon({root, frameLowerLeft, frameLowerRight});
    addPolygon({root, frameUpperLeft, frameLowerLeft});
    addPolygon({frameUpperLeft, frameUpperRight, frameLowerRight, frameLowerLeft});
    addPolygon({triangleTop, triangleRight, triangleLeft});

    pickFrameProgram->setAttribute("a_position", positions);
    pickFrameProgram->setAttribute("a_normal", normals);
    pickFrameProgram->setAttribute("a_barycoord", bcoord);
    // pickFrameProgram->setAttribute("a_edgeIsReal", edgeReal);

    size_t nFaces = 7;
    std::vector<glm::vec3> faceColor(3 * nFaces, pickColor);
    std::vector<std::array<glm::vec3, 3>> tripleColors(3 * nFaces,
                                                       std::array<glm::vec3, 3>{pickColor, pickColor, pickColor});
    pickFrameProgram->setAttribute<glm::vec3, 3>("a_vertexColors", tripleColors);
    pickFrameProgram->setAttribute<glm::vec3, 3>("a_edgeColors", tripleColors);
    pickFrameProgram->setAttribute<glm::vec3, 3>("a_halfedgeColors", tripleColors);
    pickFrameProgram->setAttribute("a_faceColor", faceColor);
  }
}

void CameraView::geometryChanged() {
  // if the programs are populated, repopulate them
  if (nodeProgram) {
    fillCameraWidgetGeometry(nodeProgram.get(), edgeProgram.get(), nullptr);
  }
  if (pickFrameProgram) {
    fillCameraWidgetGeometry(nodeProgram.get(), edgeProgram.get(), nullptr);
  }

  requestRedraw();
  QuantityStructure<CameraView>::refresh();
}

void CameraView::buildPickUI(size_t localPickID) {
  // TODO

  /*
  ImGui::TextUnformatted(("#" + std::to_string(localPickID) + "  ").c_str());
  ImGui::SameLine();
  ImGui::TextUnformatted(to_string(points[localPickID]).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildPickUI(localPickID);
  }

  ImGui::Indent(-20.);
  */
}

void CameraView::buildCustomUI() {


  ImGui::SameLine();

  { // colors
    if (ImGui::ColorEdit3("Color", &widgetColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setWidgetColor(widgetColor.get());
  }

  ImGui::Text("fov: %0.1f deg   aspect: %.2f", params.fov, params.aspectRatio);
}

void CameraView::buildCustomOptionsUI() {

  ImGui::PushItemWidth(150);

  if (displayFocalLengthUpper == -777) displayFocalLengthUpper = 2. * (*displayFocalLength.get().getValuePtr());
  if (ImGui::SliderFloat("widget focal length", displayFocalLength.get().getValuePtr(), 0, displayFocalLengthUpper,
                         "%.5f")) {
    displayFocalLength.manuallyChanged();
    geometryChanged();
    requestRedraw();
  }
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    // the upper bound for the slider is dynamically adjust to be a bit bigger than the lower bound, but only does
    // so on release of the widget (so it doesn't scaleo off to infinity)
    displayFocalLengthUpper = std::fmax(2. * (*displayFocalLength.get().getValuePtr()), 0.0001);
  }

  if (ImGui::SliderFloat("widget thickness", &displayThickness.get(), 0, 0.2, "%.5f")) {
    displayThickness.manuallyChanged();
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


// === Quantities

// Quantity default methods
/*
CameraViewQuantity::CameraViewQuantity(std::string name_, CameraView& pointCloud_, bool dominates_)
    : QuantityS<CameraView>(name_, pointCloud_, dominates_) {}


void CameraViewQuantity::buildInfoGUI(size_t pointInd) {}
*/

// === Setters and getters

CameraParameters CameraView::getCameraParameters() { return params; }

CameraView* CameraView::setDisplayFocalLength(double newVal, bool isRelative) {
  displayFocalLength = ScaledValue<float>(newVal, isRelative);
  geometryChanged();
  polyscope::requestRedraw();
  return this;
}
double CameraView::getDisplayFocalLength() { return displayFocalLength.get().asAbsolute(); }

CameraView* CameraView::setDisplayThickness(double newVal) {
  displayThickness = newVal;
  polyscope::requestRedraw();
  return this;
}
double CameraView::getDisplayThickness() { return displayThickness.get(); }

CameraView* CameraView::setWidgetColor(glm::vec3 val) {
  widgetColor = val;
  requestRedraw();
  return this;
}
glm::vec3 CameraView::getWidgetColor() { return widgetColor.get(); }

} // namespace polyscope

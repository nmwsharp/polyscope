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
      displayThickness(uniquePrefix() + "#displayThickness", 2.) {
  updateObjectSpaceBounds();
}


void CameraView::draw() {
  if (!isEnabled()) {
    return;
  }

  // Ensure we have prepared buffers
  if (nodeProgram == nullptr || frameProgram == nullptr) {
    prepare();
  }

  // Set program uniforms
  setStructureUniforms(*nodeProgram);
  setStructureUniforms(*frameProgram);
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  nodeProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  nodeProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  nodeProgram->setUniform("u_pointRadius", getDisplayFocalLength() * 0.05);
  nodeProgram->setUniform("u_baseColor", cameraFrameColor);

  frameProgram->setUniform("u_baseColor", cameraFrameColor);
  frameProgram->setUniform("u_edgeWidth", displayThickness.get());
  frameProgram->setUniform("u_edgeColor", cameraFrameColor);

  // Draw the camera view wireframe
  nodeProgram->draw();
  frameProgram->draw();


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
  if (pickProgram == nullptr) {
    preparePick();
  }

  // Set uniforms
  setStructureUniforms(*pickProgram);

  pickProgram->draw();
}

void CameraView::prepare() {

  {
    nodeProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SHADE_BASECOLOR"});
    render::engine->setMaterial(*nodeProgram, "flat");
  }

  {
    frameProgram = render::engine->requestShader("MESH", {"SHADE_BASECOLOR", "MESH_WIREFRAME", "MESH_WIREFRAME_ONLY"});
    render::engine->setMaterial(*frameProgram, "flat");
  }

  // Fill out the geometry data for the programs
  fillCameraWidgetNodeGeometry(*nodeProgram);
  fillCameraWidgetFrameGeometry(*frameProgram);
}

void CameraView::fillCameraWidgetNodeGeometry(render::ShaderProgram& nodeProgram) {

  glm::vec3 root = params.getPosition();

  { // Nodes
    std::vector<glm::vec3> allPos{root};
    nodeProgram.setAttribute("a_position", allPos);
  }
}

void CameraView::fillCameraWidgetFrameGeometry(render::ShaderProgram& p) {
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


  { // Frame

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> bcoord;
    std::vector<glm::vec3> edgeReal;

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
        glm::vec3 edgeRealV{0., 1., 0.};
        if (j == 1) {
          edgeRealV.x = 1.;
        }
        if (j + 2 == D) {
          edgeRealV.z = 1.;
        }
        edgeReal.push_back(edgeRealV);
        edgeReal.push_back(edgeRealV);
        edgeReal.push_back(edgeRealV);
      }
    };

    addPolygon({root, frameUpperRight, frameUpperLeft});
    addPolygon({root, frameLowerRight, frameUpperRight});
    addPolygon({root, frameLowerLeft, frameLowerRight});
    addPolygon({root, frameUpperLeft, frameLowerLeft});
    addPolygon({frameUpperLeft, frameUpperRight, frameLowerRight, frameLowerLeft});
    addPolygon({triangleTop, triangleRight, triangleLeft});

    p.setAttribute("a_position", positions);
    p.setAttribute("a_normal", normals);
    p.setAttribute("a_barycoord", bcoord);
    p.setAttribute("a_edgeIsReal", edgeReal);
  }
}

void CameraView::preparePick() {
  // TODO
  /*

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  pickProgram =
      render::engine->requestShader(getShaderNameForRenderMode(), addCameraViewRules({"SPHERE_PROPAGATE_COLOR"}, true),
                                    render::ShaderReplacementDefaults::Pick);

  // Fill color buffer with packed point indices
  std::vector<glm::vec3> pickColors;
  for (size_t i = pickStart; i < pickStart + pickCount; i++) {
    glm::vec3 val = pick::indToVec(i);
    pickColors.push_back(pick::indToVec(i));
  }

  // Store data in buffers
  fillGeometryBuffers(*pickProgram);
  pickProgram->setAttribute("a_color", pickColors);
  */
}


void CameraView::geometryChanged() {
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

void CameraView::buildCustomUI() {}

void CameraView::buildCustomOptionsUI() {

  if (displayFocalLengthUpper == -777) displayFocalLengthUpper = 2. * (*displayFocalLength.get().getValuePtr());
  if (ImGui::SliderFloat("widget thickness", displayFocalLength.get().getValuePtr(), 0, displayFocalLengthUpper,
                         "%.5f")) {
    displayFocalLength.manuallyChanged();
    if (nodeProgram) fillCameraWidgetNodeGeometry(*nodeProgram);
    if (frameProgram) fillCameraWidgetFrameGeometry(*frameProgram);
    requestRedraw();
  }
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    // the upper bound for the slider is dynamically adjust to be a bit bigger than the lower bound, but only does
    // so on release of the widget (so it doesn't scaleo off to infinity)
    displayFocalLengthUpper = std::fmax(2. * (*displayFocalLength.get().getValuePtr()), 0.0001);
  }

  // if (ImGui::BeginMenu("Back Face Policy")) {
  // if (ImGui::MenuItem("identical shading", NULL, backFacePolicy.get() == BackFacePolicy::Identical))
  // setBackFacePolicy(BackFacePolicy::Identical);
  // ImGui::EndMenu();
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
  frameProgram.reset();
  pickProgram.reset();
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

CameraView* CameraView::setDisplayFocalLength(double newVal, bool isRelative) {
  displayFocalLength = ScaledValue<float>(newVal, isRelative);
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

} // namespace polyscope

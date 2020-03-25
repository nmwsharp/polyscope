// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/ground_plane.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"
#include "polyscope/render/material_defs.h"

#include "imgui.h"
#include "stb_image.h"

namespace polyscope {

namespace options {
bool groundPlaneEnabled = true;
}

namespace render {

void GroundPlane::populateGroundPlaneGeometry() {

  int iP = 0;
  switch (view::upDir) {
  case view::UpDir::XUp:
    iP = 0;
    break;
  case view::UpDir::YUp:
    iP = 1;
    break;
  case view::UpDir::ZUp:
    iP = 2;
    break;
  }


  // Geometry of the ground plane, using triangles with vertices at infinity
  // clang-format off
  glm::vec4 cVert{0., 0., 0., 1.};
  glm::vec4 v1{0., 0., 0., 0.}; v1[(iP+2)%3] =  1.;
  glm::vec4 v2{0., 0., 0., 0.}; v2[(iP+1)%3] =  1.;
  glm::vec4 v3{0., 0., 0., 0.}; v3[(iP+2)%3] = -1.;
  glm::vec4 v4{0., 0., 0., 0.}; v4[(iP+1)%3] = -1.;
  
  std::vector<glm::vec4> positions = {
    cVert, v2, v1,
    cVert, v3, v2,
    cVert, v4, v3,
    cVert, v1, v4
  };
  // clang-format on

  groundPlaneProgram->setAttribute("a_position", positions);
  groundPlaneViewCached = view::upDir;
}

void GroundPlane::prepareGroundPlane() {

  // The program that draws the ground plane
  groundPlaneProgram =
      render::engine->generateShaderProgram({GROUND_PLANE_VERT_SHADER, GROUND_PLANE_FRAG_SHADER}, DrawMode::Triangles);

  populateGroundPlaneGeometry();

  { // Load the ground texture
    int w, h, comp;
    unsigned char* image = nullptr;
    image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&render::bindata_concrete[0]),
                                  render::bindata_concrete.size(), &w, &h, &comp, STBI_rgb);
    if (image == nullptr) throw std::logic_error("Failed to load material image");
    groundPlaneProgram->setTexture2D("t_ground", image, w, h, false, false, true);
		stbi_image_free(image);
  }


  { // Mirored scene buffer

    mirroredSceneColorTexture =
        render::engine->generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
    std::shared_ptr<RenderBuffer> mirroredSceneDepth =
        render::engine->generateRenderBuffer(RenderBufferType::Depth, view::bufferWidth, view::bufferHeight);

    mirroredSceneFrameBuffer = render::engine->generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    mirroredSceneFrameBuffer->addColorBuffer(mirroredSceneColorTexture);
    mirroredSceneFrameBuffer->addDepthBuffer(mirroredSceneDepth);
    mirroredSceneFrameBuffer->setDrawBuffers();

    mirroredSceneFrameBuffer->clearColor = glm::vec3{1., 1., 1.};
    mirroredSceneFrameBuffer->clearAlpha = 0.0;

    groundPlaneProgram->setTextureFromBuffer("t_mirrorImage", mirroredSceneColorTexture.get());
  }

  groundPlanePrepared = true;
}

void GroundPlane::draw() {

  // don't draw ground in planar mode
  if (view::style == view::NavigateStyle::Planar) return;

  if (!groundPlanePrepared) {
    prepareGroundPlane();
  }
  if (view::upDir != groundPlaneViewCached) {
    populateGroundPlaneGeometry();
  }

  // Get logical "up" direction to which the ground plane is oriented
  int iP = 0;
  switch (view::upDir) {
  case view::UpDir::XUp:
    iP = 0;
    break;
  case view::UpDir::YUp:
    iP = 1;
    break;
  case view::UpDir::ZUp:
    iP = 2;
    break;
  }
  glm::vec3 baseUp{0., 0., 0.};
  glm::vec3 baseForward{0., 0., 0.};
  glm::vec3 baseRight{0., 0., 0.};
  baseUp[iP] = 1.;
  baseForward[(iP + 1) % 3] = 1.;
  baseRight[(iP + 2) % 3] = 1.;

  // Location for ground plane
  double bboxBottom = std::get<0>(state::boundingBox)[iP];
  double bboxHeight = std::get<1>(state::boundingBox)[iP] - std::get<0>(state::boundingBox)[iP];
  double heightEPS = state::lengthScale * 1e-4;
  double groundHeight = bboxBottom - groundPlaneHeightFactor * bboxHeight - heightEPS;

  auto setUniforms = [&]() {
    glm::mat4 viewMat = view::getCameraViewMatrix();
    groundPlaneProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    groundPlaneProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

    glm::vec4 viewport = render::engine->getCurrentViewport();
    glm::vec2 viewportDim{viewport[2], viewport[3]};
    groundPlaneProgram->setUniform("u_viewportDim", viewportDim);

    groundPlaneProgram->setUniform("u_center", state::center);
    groundPlaneProgram->setUniform("u_basisX", baseForward);
    groundPlaneProgram->setUniform("u_basisY", baseRight);
    groundPlaneProgram->setUniform("u_basisZ", baseUp);

    float camHeight = view::getCameraWorldPosition()[iP];
    groundPlaneProgram->setUniform("u_cameraHeight", camHeight);

    groundPlaneProgram->setUniform("u_lengthScale", state::lengthScale);
    groundPlaneProgram->setUniform("u_groundHeight", groundHeight);
  };


  // Implement the mirror effect
  {
    // Render to a texture so we can sample from it on the ground
    // (use a texture 1/4 the area of the view buffer, it's supposed to be blurry anyway and this saves perf)
    // viewport size problems
    mirroredSceneFrameBuffer->resize(view::bufferWidth / 2, view::bufferHeight / 2);
    mirroredSceneFrameBuffer->setViewport(0, 0, view::bufferWidth / 2, view::bufferHeight / 2);
    render::engine->setCurrentPixelScaling(1. / 2.);

    mirroredSceneFrameBuffer->bindForRendering();
    mirroredSceneFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
    mirroredSceneFrameBuffer->clear();

    // Push a reflected view matrix
    glm::mat4 origViewMat = view::viewMat;

    glm::vec3 mirrorN = baseUp;
    glm::mat3 mirrorMat3 = glm::mat3(1.0) - 2.0f * glm::outerProduct(mirrorN, mirrorN);
    glm::vec3 tVec{0., 0., 0.};
    tVec[iP] = -groundHeight;
    glm::mat4 mirrorMat =
        glm::translate(glm::mat4(1.0), -tVec) * glm::mat4(mirrorMat3) * glm::translate(glm::mat4(1.0), tVec);
    view::viewMat = view::viewMat * mirrorMat;

    // Draw everything
    drawStructures();

    // Restore original view matrix
    view::viewMat = origViewMat;

    render::engine->bindSceneBuffer();
  }


  setUniforms();
  render::engine->setBlendMode(BlendMode::Over);
  groundPlaneProgram->draw();
  render::engine->setBlendMode(BlendMode::Disable);
}

void GroundPlane::buildGui() {

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Ground Plane")) {

    if (ImGui::Checkbox("Enabled", &options::groundPlaneEnabled)) requestRedraw();
    if (ImGui::SliderFloat("Height", &groundPlaneHeightFactor, 0.0, 1.0)) requestRedraw();

    ImGui::TreePop();
  }
}

} // namespace render
} // namespace polyscope

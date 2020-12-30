// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/ground_plane.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/material_defs.h"

#include "imgui.h"
#include "stb_image.h"

namespace polyscope {
namespace render {

// quick helper function
namespace {
std::tuple<int, float> getGroundPlaneAxisAndSign() {
  int iP = 0;
  switch (view::upDir) {
  case view::UpDir::NegXUp:
  case view::UpDir::XUp:
    iP = 0;
    break;
  case view::UpDir::NegYUp:
  case view::UpDir::YUp:
    iP = 1;
    break;
  case view::UpDir::NegZUp:
  case view::UpDir::ZUp:
    iP = 2;
    break;
  }
  float sign = 1.0;
  if (view::upDir == view::UpDir::NegXUp || view::upDir == view::UpDir::NegYUp || view::upDir == view::UpDir::NegZUp) {
    sign = -1.;
  }

  return std::tuple<int, float>{iP, sign};
}
}; // namespace

void GroundPlane::populateGroundPlaneGeometry() {

  int iP;
  float sign;
  std::tie(iP, sign) = getGroundPlaneAxisAndSign();

  // Geometry of the ground plane, using triangles with vertices at infinity
  // clang-format off
  glm::vec4 cVert{0., 0., 0., 1.};
  glm::vec4 v1{0., 0., 0., 0.}; v1[(iP+2)%3] = sign * 1.;
  glm::vec4 v2{0., 0., 0., 0.}; v2[(iP+1)%3] = sign * 1.;
  glm::vec4 v3{0., 0., 0., 0.}; v3[(iP+2)%3] = sign *-1.;
  glm::vec4 v4{0., 0., 0., 0.}; v4[(iP+1)%3] = sign *-1.;
  
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

void GroundPlane::prepare() {
  if (options::groundPlaneMode == GroundPlaneMode::None) {
    return;
  }

  // The program that draws the ground plane
  std::vector<std::string> rules;
  if (options::transparencyMode == TransparencyMode::Pretty) rules.push_back("TRANSPARENCY_PEEL_GROUND");
  switch (options::groundPlaneMode) {
  case GroundPlaneMode::None:
    break;
  case GroundPlaneMode::Tile:
    groundPlaneProgram =
        render::engine->requestShader("GROUND_PLANE_TILE", rules, render::ShaderReplacementDefaults::Process);
    break;
  case GroundPlaneMode::TileReflection:
    groundPlaneProgram =
        render::engine->requestShader("GROUND_PLANE_TILE_REFLECT", rules, render::ShaderReplacementDefaults::Process);
    break;
  case GroundPlaneMode::ShadowOnly:
    groundPlaneProgram =
        render::engine->requestShader("GROUND_PLANE_SHADOW", rules, render::ShaderReplacementDefaults::Process);
    break;
  }
  populateGroundPlaneGeometry();

  if (options::groundPlaneMode == GroundPlaneMode::Tile ||
      options::groundPlaneMode == GroundPlaneMode::TileReflection) { // Load the ground texture
    int w, h, comp;
    unsigned char* image = nullptr;
    image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&render::bindata_concrete[0]),
                                  render::bindata_concrete.size(), &w, &h, &comp, STBI_rgb);
    if (image == nullptr) throw std::logic_error("Failed to load material image");
    groundPlaneProgram->setTexture2D("t_ground", image, w, h, false, false, true);
    stbi_image_free(image);
  }

  // For all effects which will use the alternate scene buffers, prepare them
  if (options::groundPlaneMode == GroundPlaneMode::TileReflection ||
      options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {

    if (options::groundPlaneMode == GroundPlaneMode::TileReflection) {
      // only use color buffer for reflection
      sceneAltColorTexture =
          render::engine->generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
      sceneAltColorTexture->setFilterMode(FilterMode::Linear);
    }
    sceneAltDepthTexture =
        render::engine->generateTextureBuffer(TextureFormat::DEPTH24, view::bufferWidth, view::bufferHeight);


    sceneAltFrameBuffer = render::engine->generateFrameBuffer(view::bufferWidth, view::bufferHeight);
    if (options::groundPlaneMode == GroundPlaneMode::TileReflection) {
      sceneAltFrameBuffer->addColorBuffer(sceneAltColorTexture);
    }
    sceneAltFrameBuffer->addDepthBuffer(sceneAltDepthTexture);
    sceneAltFrameBuffer->setDrawBuffers();

    sceneAltFrameBuffer->clearColor = glm::vec3{1., 1., 1.};
    sceneAltFrameBuffer->clearAlpha = 0.0;
  }


  if (options::groundPlaneMode == GroundPlaneMode::TileReflection) { // Mirrored scene buffer
    groundPlaneProgram->setTextureFromBuffer("t_mirrorImage", sceneAltColorTexture.get());
  }


  if (options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {
    // Blur buffers and program
    for (int i = 0; i < 2; i++) {
      blurColorTextures[i] =
          render::engine->generateTextureBuffer(TextureFormat::RGBA16F, view::bufferWidth, view::bufferHeight);
      blurFrameBuffers[i] = render::engine->generateFrameBuffer(view::bufferWidth, view::bufferHeight);

      blurFrameBuffers[i]->addColorBuffer(blurColorTextures[i]);
      blurFrameBuffers[i]->setDrawBuffers();

      blurFrameBuffers[i]->clearColor = glm::vec3{1., 1., 1.};
      blurFrameBuffers[i]->clearAlpha = 0.0;
    }

    blurProgram = render::engine->requestShader("BLUR_RGB", {}, render::ShaderReplacementDefaults::Process);
    blurProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
    copyTexProgram = render::engine->requestShader("DEPTH_TO_MASK", {}, render::ShaderReplacementDefaults::Process);
    copyTexProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
    copyTexProgram->setTextureFromBuffer("t_depth", sceneAltDepthTexture.get());

    groundPlaneProgram->setTextureFromBuffer("t_shadow", blurColorTextures[0].get());
  }

  // Respect global effects
  if (options::transparencyMode == TransparencyMode::Pretty) {
    groundPlaneProgram->setTextureFromBuffer("t_minDepth", render::engine->sceneDepthMin.get());
  }

  groundPlanePrepared = true;
}

void GroundPlane::draw(bool isRedraw) {
  if (options::groundPlaneMode == GroundPlaneMode::None) {
    return;
  }

  // don't draw ground in planar mode
  if (view::style == view::NavigateStyle::Planar) return;

  if (!groundPlanePrepared) {
    prepare();
  }
  if (view::upDir != groundPlaneViewCached) {
    populateGroundPlaneGeometry();
  }

  // Get logical "up" direction to which the ground plane is oriented
  int iP;
  float sign;
  std::tie(iP, sign) = getGroundPlaneAxisAndSign();

  glm::vec3 baseUp{0., 0., 0.};
  glm::vec3 baseForward{0., 0., 0.};
  glm::vec3 baseRight{0., 0., 0.};
  baseUp[iP] = 1.;
  baseForward[(iP + 1) % 3] = sign;
  baseRight[(iP + 2) % 3] = sign;

  // Location for ground plane
  double bboxBottom = sign == 1.0 ? std::get<0>(state::boundingBox)[iP] : std::get<1>(state::boundingBox)[iP];
  double bboxHeight = std::get<1>(state::boundingBox)[iP] - std::get<0>(state::boundingBox)[iP];
  double heightEPS = state::lengthScale * 1e-4;
  double groundHeight = bboxBottom - sign * (options::groundPlaneHeightFactor.asAbsolute() + heightEPS);

  // Viewport
  glm::vec4 viewport = render::engine->getCurrentViewport();
  glm::vec2 viewportDim{viewport[2], viewport[3]};
  int factor = render::engine->getSSAAFactor();

  auto setUniforms = [&]() {
    glm::mat4 viewMat = view::getCameraViewMatrix();
    groundPlaneProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    groundPlaneProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));
    groundPlaneProgram->setUniform("u_viewportDim", viewportDim);

    if (options::groundPlaneMode == GroundPlaneMode::Tile ||
        options::groundPlaneMode == GroundPlaneMode::TileReflection) {
      groundPlaneProgram->setUniform("u_center", state::center);
      groundPlaneProgram->setUniform("u_basisX", baseForward);
      groundPlaneProgram->setUniform("u_basisY", baseRight);
    }

    if (options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {
      groundPlaneProgram->setUniform("u_shadowDarkness", options::shadowDarkness);
    }

    float camHeight = view::getCameraWorldPosition()[iP];
    groundPlaneProgram->setUniform("u_cameraHeight", camHeight);
    groundPlaneProgram->setUniform("u_upSign", sign);
    groundPlaneProgram->setUniform("u_basisZ", baseUp);
    groundPlaneProgram->setUniform("u_groundHeight", groundHeight);
    groundPlaneProgram->setUniform("u_lengthScale", state::lengthScale);
  };

  /*
  // For all effects which will use the alternate scene buffers, prepare them
  if (options::groundPlaneMode == GroundPlaneMode::TileReflection ||
      options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {


    sceneAltFrameBuffer->resize(factor * view::bufferWidth / 2, factor * view::bufferHeight / 2);
    sceneAltFrameBuffer->setViewport(0, 0, factor * view::bufferWidth / 2, factor * view::bufferHeight / 2);
    render::engine->setCurrentPixelScaling(factor / 2.);

    sceneAltFrameBuffer->bindForRendering();
    sceneAltFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
    sceneAltFrameBuffer->clear();
  }
  */

  // Render the scene to implement the mirror effect
  if (!isRedraw && options::groundPlaneMode == GroundPlaneMode::TileReflection) {

    // Prepare the alternate scene buffers
    // (use a texture 1/4 the area of the view buffer, it's supposed to be blurry anyway and this saves perf)
    render::engine->setBlendMode();
    render::engine->setDepthMode();
    sceneAltFrameBuffer->resize(factor * view::bufferWidth / 2, factor * view::bufferHeight / 2);
    sceneAltFrameBuffer->setViewport(0, 0, factor * view::bufferWidth / 2, factor * view::bufferHeight / 2);
    render::engine->setCurrentPixelScaling(factor / 2.);

    sceneAltFrameBuffer->bindForRendering();
    sceneAltFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
    sceneAltFrameBuffer->clear();

    // Render to a texture so we can sample from it on the ground
    sceneAltFrameBuffer->bindForRendering();

    // Push a reflected view matrix
    glm::mat4 origViewMat = view::viewMat;

    glm::vec3 mirrorN = baseUp * sign;
    glm::mat3 mirrorMat3 = glm::mat3(1.0) - 2.0f * glm::outerProduct(mirrorN, mirrorN);
    glm::vec3 tVec{0., 0., 0.};
    tVec[iP] = -groundHeight;
    glm::mat4 mirrorMat =
        glm::translate(glm::mat4(1.0), -tVec) * glm::mat4(mirrorMat3) * glm::translate(glm::mat4(1.0), tVec);
    view::viewMat = view::viewMat * mirrorMat;

    // Draw everything
    if (!render::engine->transparencyEnabled()) { // skip when transparency is turned on
      drawStructures();
    }

    // Restore original view matrix
    view::viewMat = origViewMat;
  }

  // Render the scene to implement the shadow effect
  if (!isRedraw && options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {

    // Prepare the alternate scene buffers
    render::engine->setBlendMode();
    render::engine->setDepthMode();
    sceneAltFrameBuffer->resize(factor * view::bufferWidth, factor * view::bufferHeight);
    sceneAltFrameBuffer->setViewport(0, 0, factor * view::bufferWidth, factor * view::bufferHeight);

    sceneAltFrameBuffer->bindForRendering();
    sceneAltFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
    sceneAltFrameBuffer->clear();

    // Make sure all framebuffers are the right shape
    for (int i = 0; i < 2; i++) {
      blurFrameBuffers[i]->resize(factor * view::bufferWidth, factor * view::bufferHeight);
      blurFrameBuffers[i]->setViewport(0, 0, factor * view::bufferWidth, factor * view::bufferHeight);
      blurFrameBuffers[i]->clear();
    }

    // Render to a texture so we can sample from it on the ground
    sceneAltFrameBuffer->bindForRendering();

    // Push a view matrix which projects on to the ground plane
    glm::mat4 origViewMat = view::viewMat;
    glm::mat4 projMat = glm::mat4(1.0);
    projMat[iP][iP] = 0.;
    projMat[3][iP] = groundHeight;
    view::viewMat = view::viewMat * projMat;

    // Draw everything
    render::engine->setDepthMode();
    render::engine->setBlendMode(BlendMode::Disable);
    drawStructures();

    // Copy the depth buffer to a texture (while upsampling)
    render::engine->setBlendMode(BlendMode::Disable);
    blurFrameBuffers[0]->bindForRendering();
    copyTexProgram->draw();

    // == Blur

    // Do some blur iterations (ends in same buffer it started in)
    int nBlur = options::shadowBlurIters * render::engine->getSSAAFactor();
    // int nBlur = 0;
    for (int i = 0; i < nBlur; i++) {
      // horizontal blur
      blurFrameBuffers[1]->bindForRendering();
      blurProgram->setTextureFromBuffer("t_image", blurColorTextures[0].get());
      blurProgram->setUniform("u_horizontal", 1);
      blurProgram->draw();

      // vertical blur
      blurFrameBuffers[0]->bindForRendering();
      blurProgram->setTextureFromBuffer("t_image", blurColorTextures[1].get());
      blurProgram->setUniform("u_horizontal", 0);
      blurProgram->draw();
    }

    // Restore original view matrix
    view::viewMat = origViewMat;
  }

  render::engine->bindSceneBuffer();

  // Render the ground plane
  render::engine->applyTransparencySettings();
  if (options::transparencyMode != TransparencyMode::Simple) {
    render::engine->setBlendMode(BlendMode::Disable);
  }
  setUniforms();
  groundPlaneProgram->draw();
}

void GroundPlane::buildGui() {

  auto modeName = [](const GroundPlaneMode& m) -> std::string {
    switch (m) {
    case GroundPlaneMode::None:
      return "None";
    case GroundPlaneMode::Tile:
      return "Tile";
    case GroundPlaneMode::TileReflection:
      return "Tile Reflection";
    case GroundPlaneMode::ShadowOnly:
      return "Shadow Only";
    }
    return "";
  };

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Ground Plane")) {

    ImGui::PushItemWidth(160);
    if (ImGui::BeginCombo("Mode", modeName(options::groundPlaneMode).c_str())) {
      for (GroundPlaneMode m : {GroundPlaneMode::None, GroundPlaneMode::Tile, GroundPlaneMode::TileReflection,
                                GroundPlaneMode::ShadowOnly}) {
        std::string mName = modeName(m);
        if (ImGui::Selectable(mName.c_str(), options::groundPlaneMode == m)) {
          options::groundPlaneMode = m;
          requestRedraw();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (ImGui::SliderFloat("Height", options::groundPlaneHeightFactor.getValuePtr(), -1.0, 1.0)) requestRedraw();

    switch (options::groundPlaneMode) {
    case GroundPlaneMode::None:
      break;
    case GroundPlaneMode::Tile:
      break;
    case GroundPlaneMode::TileReflection:
      break;
    case GroundPlaneMode::ShadowOnly:
      if (ImGui::SliderFloat("Shadow Darkness", &options::shadowDarkness, .0, 1.0)) requestRedraw();
      if (ImGui::InputInt("Blur Iterations", &options::shadowBlurIters, 1)) requestRedraw();
      break;
    }


    ImGui::TreePop();
  }
}

} // namespace render
} // namespace polyscope

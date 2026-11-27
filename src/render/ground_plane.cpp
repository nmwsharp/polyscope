// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
  groundPlaneProgram->setDrawCount(4 * 3);
  groundPlaneViewCached = view::upDir;
}

void GroundPlane::prepare() {

  if (options::groundPlaneMode == GroundPlaneMode::None) {
    // quick-out for the none case
    groundPlanePrepared = true;
    groundPlanePreparedMode = options::groundPlaneMode;
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
    if (image == nullptr) exception("Failed to load material image");
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
      blurColorTextures[i]->setFilterMode(FilterMode::Linear);
      blurFrameBuffers[i] = render::engine->generateFrameBuffer(view::bufferWidth, view::bufferHeight);

      blurFrameBuffers[i]->addColorBuffer(blurColorTextures[i]);
      blurFrameBuffers[i]->setDrawBuffers();

      blurFrameBuffers[i]->clearColor = glm::vec3{1., 1., 1.};
      blurFrameBuffers[i]->clearAlpha = 0.0;
    }

    blurProgram = render::engine->requestShader("BLUR_RGB", {}, render::ShaderReplacementDefaults::Process);
    blurProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
    blurProgram->setDrawCount(render::engine->screenTrianglesCoords().size());
    copyTexProgram = render::engine->requestShader("DEPTH_TO_MASK", {}, render::ShaderReplacementDefaults::Process);
    copyTexProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
    copyTexProgram->setDrawCount(render::engine->screenTrianglesCoords().size());
    copyTexProgram->setTextureFromBuffer("t_depth", sceneAltDepthTexture.get());

    groundPlaneProgram->setTextureFromBuffer("t_shadow", blurColorTextures[0].get());
  }

  // Respect global effects
  if (options::transparencyMode == TransparencyMode::Pretty) {
    groundPlaneProgram->setTextureFromBuffer("t_minDepth", render::engine->sceneDepthMin.get());
  }

  groundPlanePrepared = true;
  groundPlanePreparedMode = options::groundPlaneMode;
}

void GroundPlane::freeAllOwnedResources() {
  groundPlaneProgram.reset();
  sceneAltColorTexture.reset();
  sceneAltDepthTexture.reset();
  sceneAltFrameBuffer.reset();
  blurProgram.reset();
  copyTexProgram.reset();

  for (int i = 0; i < 2; i++) {
    blurColorTextures[i].reset();
    blurFrameBuffers[i].reset();
  }
}

void GroundPlane::draw(bool isRedraw) {
  if (options::groundPlaneMode == GroundPlaneMode::None) {
    return;
  }

  // don't draw ground in planar mode
  if (view::style == view::NavigateStyle::Planar) return;

  // don't draw the ground in Simple transparency mode
  // (there's not really any way to do so that doesn't look weird at the horizon boundary)
  if (render::engine->getTransparencyMode() == TransparencyMode::Simple) {
    return;
  }

  if (!groundPlanePrepared || groundPlanePreparedMode != options::groundPlaneMode) {
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

  double groundHeight = -777;
  switch (options::groundPlaneHeightMode) {
  case GroundPlaneHeightMode::Automatic: {
    groundHeight = bboxBottom - sign * (options::groundPlaneHeightFactor.asAbsolute() + heightEPS);
    break;
  }
  case GroundPlaneHeightMode::Manual: {
    groundHeight = options::groundPlaneHeight;
    break;
  }
  }

  // Viewport
  glm::vec4 viewport = render::engine->getCurrentViewport();
  bool hadScissor = render::engine->isScissorEnabled();
  int factor = render::engine->getSSAAFactor();
  glm::vec2 altTexCoordMax{1.f, 1.f};

  auto setUniforms = [&]() {
    glm::mat4 viewMat = view::getCameraViewMatrix();
    groundPlaneProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    groundPlaneProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));
    groundPlaneProgram->setUniform("u_viewport", viewport);

    if (options::groundPlaneMode == GroundPlaneMode::Tile ||
        options::groundPlaneMode == GroundPlaneMode::TileReflection) {
      groundPlaneProgram->setUniform("u_center", state::center());
      groundPlaneProgram->setUniform("u_basisX", baseForward);
      groundPlaneProgram->setUniform("u_basisY", baseRight);
    }

    if (options::groundPlaneMode == GroundPlaneMode::TileReflection ||
        options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {
      groundPlaneProgram->setUniform("u_texCoordMax", altTexCoordMax);
    }

    if (options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {
      groundPlaneProgram->setUniform("u_shadowDarkness", options::shadowDarkness);
    }

    switch (view::projectionMode) {
    case ProjectionMode::Perspective: {
      float camHeight = view::getCameraWorldPosition()[iP];
      groundPlaneProgram->setUniform("u_cameraHeight", camHeight);
      break;
    }
    case ProjectionMode::Orthographic: {
      glm::vec4 lookDir = glm::vec4(0, 0, 1, 0) * viewMat;
      groundPlaneProgram->setUniform("u_cameraHeight", (lookDir[iP] * state::lengthScale) + groundHeight);
      break;
    }
    }

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

  bool switchedFramebuffer = false;

  // Render the scene to implement the mirror effect
  if (options::groundPlaneMode == GroundPlaneMode::TileReflection) {
    if (render::engine->transparencyEnabled()) {
      // Reflections are skipped when transparency is enabled; ensure the mirror texture has alpha == 0
      // once (at 1x1) without re-allocating or re-binding on every viewport/frame.
      altTexCoordMax = glm::vec2{1.f, 1.f};
      if (!isRedraw && (sceneAltFrameBuffer->getSizeX() != 1 || sceneAltFrameBuffer->getSizeY() != 1)) {
        if (hadScissor) render::engine->disableScissor();
        sceneAltFrameBuffer->resize(1, 1);
        sceneAltFrameBuffer->setViewport(0, 0, 1, 1);
        sceneAltFrameBuffer->clearColor = {0.f, 0.f, 0.f};
        sceneAltFrameBuffer->clearAlpha = 0.0f;
        sceneAltFrameBuffer->clear();
        switchedFramebuffer = true;
      }
    } else {
      // Keep the alternate scene buffer at half the full-window size (allocated once per window/SSAA size)
      // and render each viewport into its (0, 0, altW, altH) subregion so differing viewport sizes never reallocate.
      unsigned int fullAltW = static_cast<unsigned int>(std::max(1, factor * view::bufferWidth / 2));
      unsigned int fullAltH = static_cast<unsigned int>(std::max(1, factor * view::bufferHeight / 2));
      unsigned int altW =
          std::min(fullAltW, static_cast<unsigned int>(std::max(1, factor * view::getActiveViewportBufferWidth() / 2)));
      unsigned int altH = std::min(
          fullAltH, static_cast<unsigned int>(std::max(1, factor * view::getActiveViewportBufferHeight() / 2)));
      altTexCoordMax = glm::vec2{static_cast<float>(altW) / fullAltW, static_cast<float>(altH) / fullAltH};

      if (!isRedraw) {
        render::engine->setBlendMode(BlendMode::AlphaOver);
        render::engine->setDepthMode(DepthMode::Less);
        sceneAltFrameBuffer->resize(fullAltW, fullAltH);
        sceneAltFrameBuffer->setViewport(0, 0, fullAltW, fullAltH);
        render::engine->setCurrentPixelScaling(factor / 2. * options::uiScale);

        sceneAltFrameBuffer->bindForRendering();
        render::engine->setViewportAndScissor(0, 0, altW, altH);
        sceneAltFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
        sceneAltFrameBuffer->clearAlpha = 0.0f;
        sceneAltFrameBuffer->clear();
        switchedFramebuffer = true;

        // Push a reflected view matrix
        glm::mat4 origViewMat = view::viewMat;

        glm::vec3 mirrorN = baseUp * sign;
        glm::mat3 mirrorMat3 = glm::mat3(1.0) - 2.0f * glm::outerProduct(mirrorN, mirrorN);
        glm::vec3 tVec{0., 0., 0.};
        tVec[iP] = -groundHeight;
        glm::mat4 mirrorMat =
            glm::translate(glm::mat4(1.0), -tVec) * glm::mat4(mirrorMat3) * glm::translate(glm::mat4(1.0), tVec);
        view::viewMat = view::viewMat * mirrorMat;

        // Flip the orientation
        render::engine->setFrontFaceCCW(!render::engine->getFrontFaceCCW());

        // Draw everything
        drawStructures();

        // Restore original values
        render::engine->setFrontFaceCCW(!render::engine->getFrontFaceCCW());
        view::viewMat = origViewMat;
      }
    }
  }

  // Render the scene to implement the shadow effect
  if (options::groundPlaneMode == GroundPlaneMode::ShadowOnly) {
    // Keep the alternate and blur buffers at fixed window-proportional sizes (allocated once per window/SSAA size)
    // and render/blur each viewport in its (0, 0, altW/blurW, altH/blurH) subregion so differing viewport sizes never
    // reallocate.
    unsigned int fullAltW = static_cast<unsigned int>(std::max(1, factor * view::bufferWidth));
    unsigned int fullAltH = static_cast<unsigned int>(std::max(1, factor * view::bufferHeight));
    unsigned int fullBlurW = std::max(1u, fullAltW / 2);
    unsigned int fullBlurH = std::max(1u, fullAltH / 2);

    unsigned int altW =
        std::min(fullAltW, static_cast<unsigned int>(std::max(1, factor * view::getActiveViewportBufferWidth())));
    unsigned int altH =
        std::min(fullAltH, static_cast<unsigned int>(std::max(1, factor * view::getActiveViewportBufferHeight())));
    unsigned int blurW = std::min(fullBlurW, std::max(1u, altW / 2));
    unsigned int blurH = std::min(fullBlurH, std::max(1u, altH / 2));

    glm::vec2 altDepthTexCoordMax{static_cast<float>(altW) / fullAltW, static_cast<float>(altH) / fullAltH};
    glm::vec2 blurTexCoordMax{static_cast<float>(blurW) / fullBlurW, static_cast<float>(blurH) / fullBlurH};
    altTexCoordMax = blurTexCoordMax;

    if (!isRedraw) {
      // Ensure blur buffers are the right shape (copyTexProgram & blurProgram overwrite every pixel with BlendMode::Disable)
      for (int i = 0; i < 2; i++) {
        blurFrameBuffers[i]->resize(fullBlurW, fullBlurH);
        blurFrameBuffers[i]->setViewport(0, 0, fullBlurW, fullBlurH);
      }

      render::engine->setBlendMode(BlendMode::AlphaOver);
      render::engine->setDepthMode(DepthMode::Less);
      sceneAltFrameBuffer->resize(fullAltW, fullAltH);
      sceneAltFrameBuffer->setViewport(0, 0, fullAltW, fullAltH);

      sceneAltFrameBuffer->bindForRendering();
      render::engine->setViewportAndScissor(0, 0, altW, altH);
      sceneAltFrameBuffer->clearColor = {view::bgColor[0], view::bgColor[1], view::bgColor[2]};
      sceneAltFrameBuffer->clear();
      switchedFramebuffer = true;

      // Push a view matrix which projects on to the ground plane
      glm::mat4 origViewMat = view::viewMat;
      glm::mat4 projMat = glm::mat4(1.0);
      projMat[iP][iP] = 0.;
      projMat[3][iP] = groundHeight;
      view::viewMat = view::viewMat * projMat;
      view::overrideClipPlanes = true; // set the clip planes manually
      view::overrideNearClipRelative = view::defaultNearClipRatio;
      view::overrideFarClipRelative = view::defaultFarClipRatio;

      // Draw everything (skip depth-peel filter: shadows come from all geometry and render into a subregion of
      // sceneAltFrameBuffer whose coordinates do not match the full-window t_minDepth texture)
      render::engine->setDepthMode(DepthMode::Less);
      render::engine->setBlendMode(BlendMode::Disable);
      render::engine->setPeelEnabled(false);
      drawStructures();
      render::engine->setPeelEnabled(true);

      // Copy the depth buffer to a texture (while downsampling into the active blur subregion)
      render::engine->setBlendMode(BlendMode::Disable);
      blurFrameBuffers[0]->bindForRendering();
      render::engine->setViewportAndScissor(0, 0, blurW, blurH);
      copyTexProgram->setUniform("u_texCoordMax", altDepthTexCoordMax);
      copyTexProgram->draw();

      // == Blur

      // Do some blur iterations (ends in same buffer it started in)
      int nBlur = options::shadowBlurIters * render::engine->getSSAAFactor();
      // int nBlur = 0;
      blurProgram->setUniform("u_texCoordMax", blurTexCoordMax);
      for (int i = 0; i < nBlur; i++) {
        // horizontal blur
        blurFrameBuffers[1]->bindForRendering();
        render::engine->setViewportAndScissor(0, 0, blurW, blurH);
        blurProgram->setTextureFromBuffer("t_image", blurColorTextures[0].get());
        blurProgram->setUniform("u_horizontal", 1);
        blurProgram->draw();

        // vertical blur
        blurFrameBuffers[0]->bindForRendering();
        render::engine->setViewportAndScissor(0, 0, blurW, blurH);
        blurProgram->setTextureFromBuffer("t_image", blurColorTextures[1].get());
        blurProgram->setUniform("u_horizontal", 0);
        blurProgram->draw();
      }

      // Restore original view matrix
      view::viewMat = origViewMat;
      view::overrideClipPlanes = false;
    }
  }

  if (switchedFramebuffer) {
    render::engine->bindSceneBuffer();
    if (hadScissor) {
      render::engine->setViewportAndScissor(static_cast<int>(viewport[0]), static_cast<int>(viewport[1]),
                                            static_cast<unsigned int>(viewport[2]),
                                            static_cast<unsigned int>(viewport[3]));
    } else {
      render::engine->disableScissor();
    }
    render::engine->applyTransparencySettings();
  }

  // Render the ground plane
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

  auto heightModeName = [](const GroundPlaneHeightMode& m) -> std::string {
    switch (m) {
    case GroundPlaneHeightMode::Automatic:
      return "Automatic";
    case GroundPlaneHeightMode::Manual:
      return "Manual";
    }
    return "";
  };

  ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Ground Plane")) {

    ImGui::PushItemWidth(160 * options::uiScale);
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

    // Height
    ImGui::PushItemWidth(80 * options::uiScale);
    switch (options::groundPlaneHeightMode) {
    case GroundPlaneHeightMode::Automatic:
      if (ImGui::SliderFloat("##HeightValue", options::groundPlaneHeightFactor.getValuePtr(), -1.0, 1.0))
        requestRedraw();
      break;
    case GroundPlaneHeightMode::Manual:
      int iP;
      float sign;
      std::tie(iP, sign) = getGroundPlaneAxisAndSign();
      double bboxBottom = sign == 1.0 ? std::get<0>(state::boundingBox)[iP] : std::get<1>(state::boundingBox)[iP];
      double bboxHeight = std::get<1>(state::boundingBox)[iP] - std::get<0>(state::boundingBox)[iP];
      if (ImGui::SliderFloat("##HeightValue", &options::groundPlaneHeight, bboxBottom - 0.5 * bboxHeight,
                             bboxBottom + bboxHeight)) {
        requestRedraw();
      }
      break;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth(100 * options::uiScale);
    if (ImGui::BeginCombo("Height##Mode", heightModeName(options::groundPlaneHeightMode).c_str())) {
      for (GroundPlaneHeightMode m : {GroundPlaneHeightMode::Automatic, GroundPlaneHeightMode::Manual}) {
        std::string mName = heightModeName(m);
        if (ImGui::Selectable(mName.c_str(), options::groundPlaneHeightMode == m)) {
          options::groundPlaneHeightMode = m;
          requestRedraw();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

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

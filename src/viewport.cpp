// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/viewport.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include <limits>

namespace polyscope {

// === View state save/restore ===

ViewStateSnapshot saveViewState() {
  ViewStateSnapshot s;
  s.viewMat = view::viewMat;
  s.fov = view::fov;
  s.viewCenter = view::viewCenter;
  s.navigateStyle = view::style;
  s.upDir = view::upDir;
  s.frontDir = view::frontDir;
  s.projectionMode = view::projectionMode;
  s.nearClip = view::nearClip;
  s.farClip = view::farClip;
  s.moveScale = view::moveScale;
  s.viewRelativeMode = view::viewRelativeMode;
  s.bgColor = view::bgColor;
  s.bufferWidth = view::bufferWidth;
  s.bufferHeight = view::bufferHeight;
  s.windowWidth = view::windowWidth;
  s.windowHeight = view::windowHeight;
  s.midflight = view::midflight;
  s.flightStartTime = view::flightStartTime;
  s.flightEndTime = view::flightEndTime;
  s.flightTargetViewR = view::flightTargetViewR;
  s.flightInitialViewR = view::flightInitialViewR;
  s.flightTargetViewT = view::flightTargetViewT;
  s.flightInitialViewT = view::flightInitialViewT;
  s.flightTargetFov = view::flightTargetFov;
  s.flightInitialFov = view::flightInitialFov;
  return s;
}

void restoreViewState(const ViewStateSnapshot& s) {
  view::viewMat = s.viewMat;
  view::fov = s.fov;
  view::viewCenter = s.viewCenter;
  view::style = s.navigateStyle;
  view::upDir = s.upDir;
  view::frontDir = s.frontDir;
  view::projectionMode = s.projectionMode;
  view::nearClip = s.nearClip;
  view::farClip = s.farClip;
  view::moveScale = s.moveScale;
  view::viewRelativeMode = s.viewRelativeMode;
  view::bgColor = s.bgColor;
  view::bufferWidth = s.bufferWidth;
  view::bufferHeight = s.bufferHeight;
  view::windowWidth = s.windowWidth;
  view::windowHeight = s.windowHeight;
  view::midflight = s.midflight;
  view::flightStartTime = s.flightStartTime;
  view::flightEndTime = s.flightEndTime;
  view::flightTargetViewR = s.flightTargetViewR;
  view::flightInitialViewR = s.flightInitialViewR;
  view::flightTargetViewT = s.flightTargetViewT;
  view::flightInitialViewT = s.flightInitialViewT;
  view::flightTargetFov = s.flightTargetFov;
  view::flightInitialFov = s.flightInitialFov;
}


// === Viewport implementation ===

Viewport::Viewport(std::string name_, int gridRow_, int gridCol_)
    : name(name_), gridRow(gridRow_), gridCol(gridCol_), viewMat(std::numeric_limits<float>::quiet_NaN()),
      fov(view::defaultFov), viewCenter(0.f, 0.f, 0.f), navigateStyle(NavigateStyle::Turntable), upDir(UpDir::YUp),
      frontDir(FrontDir::ZFront), projectionMode(ProjectionMode::Perspective), nearClip(view::defaultNearClipRatio),
      farClip(view::defaultFarClipRatio), moveScale(1.0f), viewRelativeMode(ViewRelativeMode::CenterRelative),
      bgColor{{1.0f, 1.0f, 1.0f, 0.0f}}, midflight(false), flightStartTime(-1), flightEndTime(-1), flightTargetFov(0),
      flightInitialFov(0), pixelX(0), pixelY(0), pixelWidth(0), pixelHeight(0), windowX(0), windowY(0), windowW(0),
      windowH(0) {}

Viewport::~Viewport() {}

void Viewport::pushViewState() {
  view::viewMat = viewMat;
  view::fov = fov;
  view::viewCenter = viewCenter;
  view::style = navigateStyle;
  view::upDir = upDir;
  view::frontDir = frontDir;
  view::projectionMode = projectionMode;
  view::nearClip = nearClip;
  view::farClip = farClip;
  view::moveScale = moveScale;
  view::viewRelativeMode = viewRelativeMode;
  view::bgColor = bgColor;
  view::bufferWidth = pixelWidth;
  view::bufferHeight = pixelHeight;
  view::windowWidth = windowW;
  view::windowHeight = windowH;
  view::midflight = midflight;
  view::flightStartTime = flightStartTime;
  view::flightEndTime = flightEndTime;
  view::flightTargetViewR = flightTargetViewR;
  view::flightInitialViewR = flightInitialViewR;
  view::flightTargetViewT = flightTargetViewT;
  view::flightInitialViewT = flightInitialViewT;
  view::flightTargetFov = flightTargetFov;
  view::flightInitialFov = flightInitialFov;
}

void Viewport::pullViewState() {
  // Note: buffer/window dimensions are intentionally NOT pulled back here.
  // They are layout-derived (pixelWidth/pixelHeight/windowW/windowH) and set by updateLayout(),
  // not by the view:: globals. pushViewState() writes them into the globals so that rendering
  // code sees the correct per-viewport size, but they should never flow back.
  viewMat = view::viewMat;
  fov = view::fov;
  viewCenter = view::viewCenter;
  navigateStyle = view::style;
  upDir = view::upDir;
  frontDir = view::frontDir;
  projectionMode = view::projectionMode;
  nearClip = view::nearClip;
  farClip = view::farClip;
  moveScale = view::moveScale;
  viewRelativeMode = view::viewRelativeMode;
  bgColor = view::bgColor;
  midflight = view::midflight;
  flightStartTime = view::flightStartTime;
  flightEndTime = view::flightEndTime;
  flightTargetViewR = view::flightTargetViewR;
  flightInitialViewR = view::flightInitialViewR;
  flightTargetViewT = view::flightTargetViewT;
  flightInitialViewT = view::flightInitialViewT;
  flightTargetFov = view::flightTargetFov;
  flightInitialFov = view::flightInitialFov;
}

void Viewport::updateLayout(int totalBufferW, int totalBufferH, int totalWindowW, int totalWindowH, int gridRows,
                            int gridCols) {
  int cellBufW = totalBufferW / gridCols;
  int cellBufH = totalBufferH / gridRows;
  int cellWinW = totalWindowW / gridCols;
  int cellWinH = totalWindowH / gridRows;

  // Last column/row absorbs any remainder from integer division
  pixelX = gridCol * cellBufW;
  pixelY = gridRow * cellBufH;
  pixelWidth = (gridCol == gridCols - 1) ? (totalBufferW - pixelX) : cellBufW;
  pixelHeight = (gridRow == gridRows - 1) ? (totalBufferH - pixelY) : cellBufH;

  windowX = gridCol * cellWinW;
  windowY = gridRow * cellWinH;
  windowW = (gridCol == gridCols - 1) ? (totalWindowW - windowX) : cellWinW;
  windowH = (gridRow == gridRows - 1) ? (totalWindowH - windowY) : cellWinH;
}

void Viewport::ensureBuffersAllocated() {
  if (!render::engine) return;
  if (pixelWidth <= 0 || pixelHeight <= 0) return;

  int ssaa = render::engine->getSSAAFactor();
  unsigned int w = static_cast<unsigned int>(pixelWidth) * ssaa;
  unsigned int h = static_cast<unsigned int>(pixelHeight) * ssaa;

  if (!sceneBuffer) {
    sceneColor = render::engine->generateTextureBuffer(TextureFormat::RGBA16F, w, h);
    sceneDepth = render::engine->generateTextureBuffer(TextureFormat::DEPTH24, w, h);
    sceneBuffer = render::engine->generateFrameBuffer(w, h);
    sceneBuffer->addColorBuffer(sceneColor);
    sceneBuffer->addDepthBuffer(sceneDepth);
    sceneBuffer->setDrawBuffers();

    sceneBuffer->clearColor = glm::vec3{1., 1., 1.};
    sceneBuffer->clearAlpha = 0.0;

    sceneColorFinal = render::engine->generateTextureBuffer(TextureFormat::RGBA16F, w, h);
    sceneBufferFinal = render::engine->generateFrameBuffer(w, h);
    sceneBufferFinal->addColorBuffer(sceneColorFinal);
    sceneBufferFinal->setDrawBuffers();

    sceneBufferFinal->clearColor = glm::vec3{1., 1., 1.};
    sceneBufferFinal->clearAlpha = 0.0;

    sceneDepthMin = render::engine->generateTextureBuffer(TextureFormat::DEPTH24, w, h);
    sceneDepthMinFrame = render::engine->generateFrameBuffer(w, h);
    sceneDepthMinFrame->addDepthBuffer(sceneDepthMin);
    sceneDepthMinFrame->setDrawBuffers();
    sceneDepthMinFrame->clearDepth = 0.0;

    // Set viewports on all framebuffers (required by the GL backend before binding)
    sceneBuffer->setViewport(0, 0, w, h);
    sceneBufferFinal->setViewport(0, 0, w, h);
    sceneDepthMinFrame->setViewport(0, 0, w, h);

    // Create per-viewport compositePeel shader for Pretty transparency
    compositePeel = render::engine->requestShader("COMPOSITE_PEEL", {}, render::ShaderReplacementDefaults::Process);
    compositePeel->setAttribute("a_position", render::engine->screenTrianglesCoords());
    compositePeel->setTextureFromBuffer("t_image", sceneColor.get());
  }
}

void Viewport::resizeBuffers() {
  if (!sceneBuffer) return;
  if (pixelWidth <= 0 || pixelHeight <= 0) return;

  unsigned int w = static_cast<unsigned int>(pixelWidth);
  unsigned int h = static_cast<unsigned int>(pixelHeight);

  int ssaa = render::engine->getSSAAFactor();
  unsigned int targetW = ssaa * w;
  unsigned int targetH = ssaa * h;

  // Early-out if size hasn't changed (avoid GPU reallocation every frame)
  if (sceneBuffer->getSizeX() == targetW && sceneBuffer->getSizeY() == targetH) return;

  sceneBuffer->resize(targetW, targetH);
  sceneBufferFinal->resize(targetW, targetH);
  sceneDepthMinFrame->resize(targetW, targetH);

  sceneBuffer->setViewport(0, 0, targetW, targetH);
  sceneBufferFinal->setViewport(0, 0, targetW, targetH);
  sceneDepthMinFrame->setViewport(0, 0, targetW, targetH);
}

void Viewport::resetCameraToHomeView() {
  // Push this viewport's state, compute home view, pull it back
  ViewStateSnapshot saved = saveViewState();
  pushViewState();
  view::resetCameraToHomeView();
  pullViewState();
  restoreViewState(saved);
}

void Viewport::setNavigateStyle(NavigateStyle newStyle, bool animateFlight) {
  ViewStateSnapshot saved = saveViewState();
  pushViewState();
  view::setNavigateStyle(newStyle, animateFlight);
  pullViewState();
  restoreViewState(saved);
}

void Viewport::setUpDir(UpDir newUpDir, bool animateFlight) {
  ViewStateSnapshot saved = saveViewState();
  pushViewState();
  view::setUpDir(newUpDir, animateFlight);
  pullViewState();
  restoreViewState(saved);
}

bool Viewport::containsScreenCoords(float x, float y) const {
  // Screen coords: (0,0) at top-left, y increases downward
  // windowY is in screen coords from top
  return x >= static_cast<float>(windowX) && x < static_cast<float>(windowX + windowW) &&
         y >= static_cast<float>(windowY) && y < static_cast<float>(windowY + windowH);
}

} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "imgui.h"
#include "implot.h"

#include "polyscope/options.h"
#include "polyscope/pick.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"
#include "polyscope/view.h"

#include "stb_image.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace polyscope {

// Note: Storage for global members lives in state.cpp and options.cpp

// Helpers
namespace {

// === Implement the context stack

// The context stack should _always_ have at least one context in it. The lowest context is the one created at
// initialization.
struct ContextEntry {
  ImGuiContext* context;
  ImPlotContext* plotContext;
  std ::function<void()> callback;
  bool drawDefaultUI;
};
std::vector<ContextEntry> contextStack;
int frameTickStack = 0;

bool redrawNextFrame = true;
bool unshowRequested = false;

// Some state about imgui windows to stack them
float imguiStackMargin = 10;
float lastWindowHeightPolyscope = 200;
float lastWindowHeightUser = 200;
constexpr float INITIAL_LEFT_WINDOWS_WIDTH = 305;
constexpr float INITIAL_RIGHT_WINDOWS_WIDTH = 500;
float leftWindowsWidth = -1.;
float rightWindowsWidth = -1.;

auto lastMainLoopIterTime = std::chrono::steady_clock::now();

const std::string prefsFilename = ".polyscope.ini";

void readPrefsFile() {

  try {

    std::ifstream inStream(prefsFilename);
    if (inStream) {

      json prefsJSON;
      inStream >> prefsJSON;

      // Set values
      // Do some basic validation on the sizes first to work around bugs with bogus values getting written to init file
      if (view::windowWidth == -1 && prefsJSON.count("windowWidth") > 0) { // only load if not already set
        int val = prefsJSON["windowWidth"];
        if (val >= 64 && val < 10000) view::windowWidth = val;
      }
      if (view::windowHeight == -1 && prefsJSON.count("windowHeight") > 0) { // only load if not already set
        int val = prefsJSON["windowHeight"];
        if (val >= 64 && val < 10000) view::windowHeight = val;
      }
      if (prefsJSON.count("windowPosX") > 0) {
        int val = prefsJSON["windowPosX"];
        if (val >= 0 && val < 10000) view::initWindowPosX = val;
      }
      if (prefsJSON.count("windowPosY") > 0) {
        int val = prefsJSON["windowPosY"];
        if (val >= 0 && val < 10000) view::initWindowPosY = val;
      }
      if (prefsJSON.count("uiScale") > 0) {
        float val = prefsJSON["uiScale"];
        if (val >= 0.25 && val <= 4.0) options::uiScale = val;
      }
    }

  }
  // We never really care if something goes wrong while loading preferences, so eat all exceptions
  catch (...) {
    polyscope::warning("Parsing of prefs file .polyscope.ini failed");
  }
}

void writePrefsFile() {

  // Update values as needed
  int posX, posY;
  std::tie(posX, posY) = render::engine->getWindowPos();
  int windowWidth = view::windowWidth;
  int windowHeight = view::windowHeight;
  float uiScale = options::uiScale;

  // Validate values. Don't write the prefs file if any of these values are obviously bogus (this seems to happen at
  // least on Windows when the application is minimzed)
  bool valuesValid = true;
  valuesValid &= posX >= 0 && posX < 10000;
  valuesValid &= posY >= 0 && posY < 10000;
  valuesValid &= windowWidth >= 64 && windowWidth < 10000;
  valuesValid &= windowHeight >= 64 && windowHeight < 10000;
  valuesValid &= uiScale >= 0.25 && uiScale <= 4.;
  if (!valuesValid) return;

  // Build json object
  // clang-format off
  json prefsJSON = {
      {"windowWidth", windowWidth}, 
      {"windowHeight", windowHeight}, 
      {"windowPosX", posX},
      {"windowPosY", posY},         
      {"uiScale", uiScale},
  };
  // clang-format on

  // Write out json object
  std::ofstream o(prefsFilename);
  o << std::setw(4) << prefsJSON << std::endl;
}

void setInitialWindowWidths() {
  leftWindowsWidth = INITIAL_LEFT_WINDOWS_WIDTH * options::uiScale;
  rightWindowsWidth = INITIAL_RIGHT_WINDOWS_WIDTH * options::uiScale;
}

void ensureWindowWidthsSet() {
  if (leftWindowsWidth <= 0. || rightWindowsWidth <= 0.) {
    setInitialWindowWidths();
  }
}

// Helper to get a structure map

std::map<std::string, std::unique_ptr<Structure>>& getStructureMapCreateIfNeeded(std::string typeName) {
  if (state::structures.find(typeName) == state::structures.end()) {
    state::structures[typeName] = std::map<std::string, std::unique_ptr<Structure>>();
  }
  return state::structures[typeName];
}

} // namespace

// === Core global functions

void init(std::string backend) {
  if (isInitialized()) {
    if (backend != state::backend) {
      exception("re-initializing with different backend is not supported");
    }
    // otherwise silently allow multiple-init
    return;
  }

  info(5, "Initializing Polyscope");

  state::backend = backend;

  if (options::usePrefsFile) {
    readPrefsFile();
  }
  if (view::windowWidth == -1) view::windowWidth = view::defaultWindowWidth;
  if (view::windowHeight == -1) view::windowHeight = view::defaultWindowHeight;

  // Initialize the rendering engine
  render::initializeRenderEngine(backend);

  // Initialie ImGUI
  IMGUI_CHECKVERSION();
  render::engine->initializeImGui();

  // Create an initial context based context. Note that calling show() never actually uses this context, because it
  // pushes a new one each time. But using frameTick() may use this context.
  contextStack.push_back(ContextEntry{ImGui::GetCurrentContext(), ImPlot::GetCurrentContext(), nullptr, true});

  view::invalidateView();

  state::initialized = true;
  state::doDefaultMouseInteraction = true;
}

void checkInitialized() {
  if (!state::initialized) {
    exception("Polyscope has not been initialized");
  }
}

bool isInitialized() { return state::initialized; }

void pushContext(std::function<void()> callbackFunction, bool drawDefaultUI) {

  // WARNING: code duplicated here and in screenshot.cpp

  // Create a new context and push it on to the stack
  ImGuiContext* newContext = ImGui::CreateContext();
  ImPlotContext* newPlotContext = ImPlot::CreateContext();
  ImGuiIO& oldIO = ImGui::GetIO(); // used to GLFW + OpenGL data to the new IO object
#ifdef IMGUI_HAS_DOCK
  ImGuiPlatformIO& oldPlatformIO = ImGui::GetPlatformIO();
#endif
  ImGui::SetCurrentContext(newContext);
  ImPlot::SetCurrentContext(newPlotContext);
#ifdef IMGUI_HAS_DOCK
  // Propagate GLFW window handle to new context
  ImGui::GetMainViewport()->PlatformHandle = oldPlatformIO.Viewports[0]->PlatformHandle;
#endif
  ImGui::GetIO().BackendPlatformUserData = oldIO.BackendPlatformUserData;
  ImGui::GetIO().BackendRendererUserData = oldIO.BackendRendererUserData;

  render::engine->configureImGui();


  contextStack.push_back(ContextEntry{newContext, newPlotContext, callbackFunction, drawDefaultUI});

  if (contextStack.size() > 50) {
    // Catch bugs with nested show()
    exception("Uh oh, polyscope::show() was recusively MANY times (depth > 50), this is probably a bug. Perhaps "
              "you are accidentally calling show() every time polyscope::userCallback executes?");
  };

  // Make sure the window is visible
  render::engine->showWindow();

  // Re-enter main loop until the context has been popped
  size_t currentContextStackSize = contextStack.size();
  while (contextStack.size() >= currentContextStackSize) {

    // The windowing system will let the main loop busy-loop on some platforms. Make sure that doesn't happen.
    if (options::maxFPS != -1) {
      auto currTime = std::chrono::steady_clock::now();
      long microsecPerLoop = 1000000 / options::maxFPS;
      microsecPerLoop = (95 * microsecPerLoop) / 100; // give a little slack so we actually hit target fps
      while (std::chrono::duration_cast<std::chrono::microseconds>(currTime - lastMainLoopIterTime).count() <
             microsecPerLoop) {
        std::this_thread::yield();
        currTime = std::chrono::steady_clock::now();
      }
    }
    lastMainLoopIterTime = std::chrono::steady_clock::now();

    mainLoopIteration();

    // auto-exit if the window is closed
    if (render::engine->windowRequestsClose()) {
      popContext();
    }
  }

  // WARNING: code duplicated here and in screenshot.cpp
  // Workaround overzealous ImGui assertion before destroying any inner context
  // https://github.com/ocornut/imgui/pull/7175
  ImGui::SetCurrentContext(newContext);
  ImPlot::SetCurrentContext(newPlotContext);
  ImGui::GetIO().BackendPlatformUserData = nullptr;
  ImGui::GetIO().BackendRendererUserData = nullptr;

  ImPlot::DestroyContext(newPlotContext);
  ImGui::DestroyContext(newContext);

  // Restore the previous context, if there was one
  if (!contextStack.empty()) {
    ImGui::SetCurrentContext(contextStack.back().context);
    ImPlot::SetCurrentContext(contextStack.back().plotContext);
  }
}


void popContext() {
  if (contextStack.empty()) {
    exception("Called popContext() too many times");
    return;
  }
  contextStack.pop_back();
}

ImGuiContext* getCurrentContext() { return contextStack.empty() ? nullptr : contextStack.back().context; }

void frameTick() {
  checkInitialized();

  // Do some sanity-checking around control flow and use of frameTick() / show()
  if (contextStack.size() > 1) {
    exception("Do not call frameTick() while show() is already looping the main loop.");
  }
  if (frameTickStack > 0) {
    exception("You called frameTick() while a previous call was in the midst of executing. Do not re-enter frameTick() "
              "or call it recursively.");
  }
  frameTickStack++;

  // Make sure we're visible
  render::engine->showWindow();

  // All-imporant main loop iteration
  mainLoopIteration();

  frameTickStack--;
}

void requestRedraw() { redrawNextFrame = true; }
bool redrawRequested() { return redrawNextFrame; }

void drawStructures() {

  // Draw all off the structures registered with polyscope

  for (auto& catMap : state::structures) {
    for (auto& s : catMap.second) {
      s.second->draw();
    }
  }

  // Also render any slice plane geometry
  for (std::unique_ptr<SlicePlane>& s : state::slicePlanes) {
    s->drawGeometry();
  }
}

void drawStructuresDelayed() {
  // "delayed" drawing allows structures to render things which should be rendered after most of the scene has been
  // drawn
  for (auto& catMap : state::structures) {
    for (auto& s : catMap.second) {
      s.second->drawDelayed();
    }
  }
}

namespace {

float dragDistSinceLastRelease = 0.0;

void processInputEvents() {
  ImGuiIO& io = ImGui::GetIO();

  // RECALL: in ImGUI language, on MacOS "ctrl" == "cmd", so all the options
  // below referring to ctrl really mean cmd on MacOS.

  // If any mouse button is pressed, trigger a redraw
  if (ImGui::IsAnyMouseDown()) {
    requestRedraw();
  }

  bool widgetCapturedMouse = false;

  // Handle scroll events for 3D view
  if (state::doDefaultMouseInteraction) {

    for (WeakHandle<Widget> wHandle : state::widgets) {
      if (wHandle.isValid()) {
        Widget& w = wHandle.get();
        widgetCapturedMouse = w.interact();
        if (widgetCapturedMouse) {
          break;
        }
      }
    }

    // === Mouse inputs
    if (!io.WantCaptureMouse && !widgetCapturedMouse) {

      { // Process scroll via "mouse wheel" (which might be a touchpad)
        double xoffset = io.MouseWheelH;
        double yoffset = io.MouseWheel;

        if (xoffset != 0 || yoffset != 0) {
          requestRedraw();

          // On some setups, shift flips the scroll direction, so take the max
          // scrolling in any direction
          double maxScroll = xoffset;
          if (std::abs(yoffset) > std::abs(xoffset)) {
            maxScroll = yoffset;
          }

          // Pass camera commands to the camera
          if (maxScroll != 0.0) {
            bool scrollClipPlane = io.KeyShift && !io.KeyCtrl;
            bool relativeZoom = io.KeyShift && io.KeyCtrl;

            if (scrollClipPlane) {
              view::processClipPlaneShift(maxScroll);
            } else {
              view::processZoom(maxScroll, relativeZoom);
            }
          }
        }
      }


      { // Process drags
        bool dragLeft = ImGui::IsMouseDragging(0);
        bool dragRight = !dragLeft && ImGui::IsMouseDragging(1); // left takes priority, so only one can be true
        if (dragLeft || dragRight) {

          glm::vec2 dragDelta{io.MouseDelta.x / view::windowWidth, -io.MouseDelta.y / view::windowHeight};
          dragDistSinceLastRelease += std::abs(dragDelta.x);
          dragDistSinceLastRelease += std::abs(dragDelta.y);

          // exactly one of these will be true
          bool isRotate = dragLeft && !io.KeyShift && !io.KeyCtrl;
          bool isTranslate = (dragLeft && io.KeyShift && !io.KeyCtrl) || dragRight;
          bool isDragZoom = dragLeft && io.KeyShift && io.KeyCtrl;

          if (isDragZoom) {
            view::processZoom(dragDelta.y * 5, true);
          }
          if (isRotate) {
            glm::vec2 currPos{io.MousePos.x / view::windowWidth,
                              (view::windowHeight - io.MousePos.y) / view::windowHeight};
            currPos = (currPos * 2.0f) - glm::vec2{1.0, 1.0};
            if (std::abs(currPos.x) <= 1.0 && std::abs(currPos.y) <= 1.0) {
              view::processRotate(currPos - 2.0f * dragDelta, currPos);
            }
          }
          if (isTranslate) {
            view::processTranslate(dragDelta);
          }
        }
      }

      { // Click picks
        float dragIgnoreThreshold = 0.01;
        bool anyModifierHeld = io.KeyShift || io.KeyCtrl || io.KeyAlt;
        bool ctrlShiftHeld = io.KeyShift && io.KeyCtrl;

        if (!anyModifierHeld && io.MouseReleased[0]) {

          // Don't pick at the end of a long drag
          if (dragDistSinceLastRelease < dragIgnoreThreshold) {
            glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
            PickResult pickResult = pickAtScreenCoords(screenCoords);
            setSelection(pickResult);
          }
        }

        // Clear pick
        if (!anyModifierHeld && io.MouseReleased[1]) {
          if (dragDistSinceLastRelease < dragIgnoreThreshold) {
            resetSelection();
          }
          dragDistSinceLastRelease = 0.0;
        }

        // Ctrl-shift left-click to set new center
        if (ctrlShiftHeld && io.MouseReleased[0]) {
          if (dragDistSinceLastRelease < dragIgnoreThreshold) {
            glm::vec2 screenCoords{io.MousePos.x, io.MousePos.y};
            view::processSetCenter(screenCoords);
          }
        }
      }
    }
  }

  // Reset the drag distance after any release
  if (io.MouseReleased[0]) {
    dragDistSinceLastRelease = 0.0;
  }

  // === Key-press inputs
  if (!io.WantCaptureKeyboard) {
    view::processKeyboardNavigation(io);
  }
}


void renderSlicePlanes() {
  for (std::unique_ptr<SlicePlane>& s : state::slicePlanes) {
    s->draw();
  }
}

void renderScene() {

  render::engine->applyTransparencySettings();

  render::engine->sceneBuffer->clearColor = {0., 0., 0.};
  render::engine->sceneBuffer->clearAlpha = 0.;
  render::engine->sceneBuffer->clear();

  if (!render::engine->bindSceneBuffer()) return;

  // If a view has never been set, this will set it to the home view
  view::ensureViewValid();

  if (!options::renderScene) return;

  if (render::engine->getTransparencyMode() == TransparencyMode::Pretty) {
    // Special depth peeling case: multiple render passes
    // We will perform several "peeled" rounds of rendering in to the usual scene buffer. After each, we will manually
    // composite in to the final scene buffer.


    // Clear the final buffer explicitly since we will gradually composite in to it rather than just blitting directly
    // as in normal rendering.
    render::engine->sceneBufferFinal->clearColor = glm::vec3{0., 0., 0.};
    render::engine->sceneBufferFinal->clearAlpha = 0;
    render::engine->sceneBufferFinal->clear();

    render::engine->setDepthMode(DepthMode::Less); // we need depth to be enabled for the clear below to do anything
    render::engine->sceneDepthMinFrame->clear();


    for (int iPass = 0; iPass < options::transparencyRenderPasses; iPass++) {

      render::engine->bindSceneBuffer();
      render::engine->clearSceneBuffer();

      render::engine->applyTransparencySettings();
      drawStructures();

      // Draw ground plane, slicers, etc
      bool isRedraw = iPass > 0;
      render::engine->groundPlane.draw(isRedraw);
      if (!isRedraw) {
        // Only on first pass (kinda weird, but works out, and doesn't really matter)
        renderSlicePlanes();
        render::engine->applyTransparencySettings();
        drawStructuresDelayed();
      }

      // Composite the result of this pass in to the result buffer
      render::engine->sceneBufferFinal->bind();
      render::engine->setDepthMode(DepthMode::Disable);
      render::engine->setBlendMode(BlendMode::AlphaUnder);
      render::engine->compositePeel->draw();

      // Update the minimum depth texture
      render::engine->updateMinDepthTexture();
    }


  } else {
    // Normal case: single render pass

    render::engine->applyTransparencySettings();
    drawStructures();

    render::engine->groundPlane.draw();
    renderSlicePlanes();

    render::engine->applyTransparencySettings();
    drawStructuresDelayed();

    render::engine->sceneBuffer->blitTo(render::engine->sceneBufferFinal.get());
  }
}

void renderSceneToScreen() {
  render::engine->bindDisplay();
  if (options::debugDrawPickBuffer) {
    // special debug draw
    pick::evaluatePickQuery(-1, -1); // populate the buffer
    render::engine->pickFramebuffer->blitTo(render::engine->displayBuffer.get());
  } else {
    render::engine->applyLightingTransform(render::engine->sceneColorFinal);
  }
}

void purgeWidgets() {
  // remove any widget objects which are no longer defined
  state::widgets.erase(std::remove_if(state::widgets.begin(), state::widgets.end(),
                                      [](const WeakHandle<Widget>& w) { return !w.isValid(); }),
                       state::widgets.end());
}

void userGuiBegin() {
  ensureWindowWidthsSet();

  ImVec2 userGuiLoc;
  if (options::userGuiIsOnRightSide) {
    // right side
    userGuiLoc = ImVec2(view::windowWidth - (rightWindowsWidth + imguiStackMargin), imguiStackMargin);
    ImGui::SetNextWindowSize(ImVec2(rightWindowsWidth, 0.));
  } else {
    // left side
    if (options::buildDefaultGuiPanels) {
      userGuiLoc = ImVec2(leftWindowsWidth + 3 * imguiStackMargin, imguiStackMargin);
    } else {
      userGuiLoc = ImVec2(imguiStackMargin, imguiStackMargin);
    }
  }

  ImGui::PushID("user_callback");
  ImGui::SetNextWindowPos(userGuiLoc);

  ImGui::Begin("##Command UI", nullptr);
}

void userGuiEnd() {

  if (options::userGuiIsOnRightSide) {
    rightWindowsWidth = INITIAL_RIGHT_WINDOWS_WIDTH * options::uiScale;
    lastWindowHeightUser = imguiStackMargin + ImGui::GetWindowHeight();
  } else {
    lastWindowHeightUser = 0;
  }
  ImGui::End();
  ImGui::PopID();
}

} // namespace

void buildPolyscopeGui() {
  ensureWindowWidthsSet();

  // Create window
  static bool showPolyscopeWindow = true;
  ImGui::SetNextWindowPos(ImVec2(imguiStackMargin, imguiStackMargin));
  ImGui::SetNextWindowSize(ImVec2(leftWindowsWidth, 0.));

  ImGui::Begin("Polyscope", &showPolyscopeWindow);

  if (ImGui::Button("Reset View")) {
    view::flyToHomeView();
  }
  ImGui::SameLine();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 0.0f));
  if (ImGui::Button("Screenshot")) {
    ScreenshotOptions options;
    options.transparentBackground = options::screenshotTransparency;
    options.includeUI = options::screenshotWithImGuiUI;
    screenshot(options);
  }
  ImGui::SameLine();
  if (ImGui::ArrowButton("##Option", ImGuiDir_Down)) {
    ImGui::OpenPopup("ScreenshotOptionsPopup");
  }
  ImGui::PopStyleVar();
  if (ImGui::BeginPopup("ScreenshotOptionsPopup")) {

    ImGui::Checkbox("with transparency", &options::screenshotTransparency);
    ImGui::Checkbox("with UI", &options::screenshotWithImGuiUI);

    if (ImGui::BeginMenu("file format")) {
      if (ImGui::MenuItem(".png", NULL, options::screenshotExtension == ".png")) options::screenshotExtension = ".png";
      if (ImGui::MenuItem(".jpg", NULL, options::screenshotExtension == ".jpg")) options::screenshotExtension = ".jpg";
      ImGui::EndMenu();
    }

    ImGui::EndPopup();
  }


  ImGui::SameLine();
  if (ImGui::Button("Controls")) {
    // do nothing, just want hover state
  }
  if (ImGui::IsItemHovered()) {

    ImGui::SetNextWindowPos(ImVec2(2 * imguiStackMargin + leftWindowsWidth, imguiStackMargin));
    ImGui::SetNextWindowSize(ImVec2(0., 0.));

    // clang-format off
		ImGui::Begin("Controls", NULL, ImGuiWindowFlags_NoTitleBar);
    ImGui::TextUnformatted("View Navigation:");
			ImGui::TextUnformatted("     Rotate: [left click drag]");
			ImGui::TextUnformatted("     Translate: [shift] + [left click drag] OR [right click drag]");
			ImGui::TextUnformatted("     Zoom: [scroll] OR [ctrl/cmd] + [shift] + [left click drag]");
			ImGui::TextUnformatted("   Use [ctrl/cmd-c] and [ctrl/cmd-v] to save and restore camera poses");
			ImGui::TextUnformatted("     via the clipboard.");
			ImGui::TextUnformatted("   Hold [ctrl/cmd] + [shift] and [left click] in the scene to set the");
			ImGui::TextUnformatted("     orbit center.");
			ImGui::TextUnformatted("   Hold [ctrl/cmd] + [shift] and scroll to zoom towards the center.");
      ImGui::TextUnformatted("\nMenu Navigation:");
			ImGui::TextUnformatted("   Menu headers with a '>' can be clicked to collapse and expand.");
			ImGui::TextUnformatted("   Use [ctrl/cmd] + [left click] to manually enter any numeric value");
			ImGui::TextUnformatted("     via the keyboard.");
			ImGui::TextUnformatted("   Press [space] to dismiss popup dialogs.");
		ImGui::TextUnformatted("\nSelection:");
			ImGui::TextUnformatted("   Select elements of a structure with [left click]. Data from");
			ImGui::TextUnformatted("     that element will be shown on the right. Use [right click]");
			ImGui::TextUnformatted("     to clear the selection.");
		ImGui::End();
    // clang-format on
  }

  // View options tree
  view::buildViewGui();

  // Appearance options tree
  render::engine->buildEngineGui();

  // Render options tree
  ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Render")) {

    // fps
    ImGui::Text("Rolling: %.1f ms/frame (%.1f fps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Last: %.1f ms/frame (%.1f fps)", ImGui::GetIO().DeltaTime * 1000.f, 1.f / ImGui::GetIO().DeltaTime);

    ImGui::PushItemWidth(40 * options::uiScale);
    if (ImGui::InputInt("max fps", &options::maxFPS, 0)) {
      if (options::maxFPS < 1 && options::maxFPS != -1) {
        options::maxFPS = -1;
      }
    }

    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Checkbox("vsync", &options::enableVSync);

    ImGui::TreePop();
  }

  ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Debug")) {

    if (ImGui::Button("Force refresh")) {
      refresh();
    }
    ImGui::Checkbox("Show pick buffer", &options::debugDrawPickBuffer);
    ImGui::Checkbox("Always redraw", &options::alwaysRedraw);

    static bool showDebugTextures = false;
    ImGui::Checkbox("Show debug textures", &showDebugTextures);
    if (showDebugTextures) {
      render::engine->showTextureInImGuiWindow("Scene", render::engine->sceneColor.get());
      render::engine->showTextureInImGuiWindow("Scene Final", render::engine->sceneColorFinal.get());
    }
    ImGui::TreePop();
  }


  lastWindowHeightPolyscope = imguiStackMargin + ImGui::GetWindowHeight();
  leftWindowsWidth = ImGui::GetWindowWidth();

  ImGui::End();
}

void buildStructureGui() {
  ensureWindowWidthsSet();

  // Create window
  static bool showStructureWindow = true;

  ImGui::SetNextWindowPos(ImVec2(imguiStackMargin, lastWindowHeightPolyscope + 2 * imguiStackMargin));
  ImGui::SetNextWindowSize(
      ImVec2(leftWindowsWidth, view::windowHeight - lastWindowHeightPolyscope - 3 * imguiStackMargin));
  ImGui::Begin("Structures", &showStructureWindow);

  // only show groups if there are any
  if (state::groups.size() > 0) {
    if (ImGui::CollapsingHeader("Groups", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (auto& x : state::groups) {
        if (x.second->isRootGroup()) {
          x.second->buildUI();
        }
      }
    }
  }

  // groups have an option to hide structures from this list; assemble a list of structures to skip
  std::unordered_set<Structure*> structuresToSkip;
  for (auto& x : state::groups) {
    x.second->appendStructuresToSkip(structuresToSkip);
  }


  for (auto& catMapEntry : state::structures) {
    std::string catName = catMapEntry.first;

    std::map<std::string, std::unique_ptr<Structure>>& structureMap = catMapEntry.second;

    ImGui::PushID(catName.c_str()); // ensure there are no conflicts with
                                    // identically-named labels

    // Build the structure's UI
    ImGui::SetNextItemOpen(structureMap.size() > 0, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader((catName + " (" + std::to_string(structureMap.size()) + ")").c_str())) {
      // Draw shared GUI elements for all instances of the structure
      if (structureMap.size() > 0) {
        structureMap.begin()->second->buildSharedStructureUI();
      }

      int32_t skipCount = 0;
      for (auto& x : structureMap) {
        ImGui::SetNextItemOpen(structureMap.size() <= 8,
                               ImGuiCond_FirstUseEver); // closed by default if more than 8

        if (structuresToSkip.find(x.second.get()) != structuresToSkip.end()) {
          skipCount++;
          continue;
        }

        x.second->buildUI();
      }

      if (skipCount > 0) {
        ImGui::Text("  (skipped %d hidden structures)", skipCount);
      }
    }

    ImGui::PopID();
  }

  leftWindowsWidth = ImGui::GetWindowWidth();

  ImGui::End();
}

void buildPickGui() {
  ensureWindowWidthsSet();

  if (haveSelection()) {

    ImGui::SetNextWindowPos(ImVec2(view::windowWidth - (rightWindowsWidth + imguiStackMargin),
                                   2 * imguiStackMargin + lastWindowHeightUser));
    ImGui::SetNextWindowSize(ImVec2(rightWindowsWidth, 0.));

    ImGui::Begin("Selection", nullptr);
    PickResult selection = getSelection();


    ImGui::Text("screen coordinates: (%.2f,%.2f)  depth: %g", selection.screenCoords.x, selection.screenCoords.y,
                selection.depth);
    ImGui::Text("world position: <%g, %g, %g>", selection.position.x, selection.position.y, selection.position.z);
    ImGui::NewLine();

    ImGui::TextUnformatted((selection.structureType + ": " + selection.structureName).c_str());
    ImGui::Separator();

    if (selection.structureHandle.isValid()) {
      selection.structureHandle.get().buildPickUI(selection);
    } else {
      // this is a paranoid check, it _should_ never happen since we
      // clear the selection when a structure is deleted
      ImGui::TextUnformatted("ERROR: INVALID STRUCTURE");
    }

    rightWindowsWidth = ImGui::GetWindowWidth();
    ImGui::End();
  }
}

void buildUserGuiAndInvokeCallback() {

  if (!options::invokeUserCallbackForNestedShow && (contextStack.size() + frameTickStack) > 2) {
    return;
  }

  if (state::userCallback) {

    bool beganUserGUI = false;
    if (options::buildGui && options::openImGuiWindowForUserCallback) {
      userGuiBegin();
      beganUserGUI = true;
    }

    state::userCallback();

    if (beganUserGUI) {
      userGuiEnd();
    } else {
      lastWindowHeightUser = imguiStackMargin;
    }

  } else {
    lastWindowHeightUser = imguiStackMargin;
  }
}

void draw(bool withUI, bool withContextCallback) {
  processLazyProperties();

  // Update buffer and context
  render::engine->makeContextCurrent();
  render::engine->bindDisplay();
  render::engine->setBackgroundColor({0., 0., 0.});
  render::engine->setBackgroundAlpha(0);
  render::engine->clearDisplay();

  if (withUI) {
    render::engine->ImGuiNewFrame();

    processInputEvents();
    view::updateFlight();
    showDelayedWarnings();
  }

  // Build the GUI components
  if (withUI) {
    if (contextStack.back().drawDefaultUI) {

      // Note: It is important to build the user GUI first, because it is likely that callbacks there will modify
      // polyscope data. If we do these modifications happen later in the render cycle, they might invalidate data which
      // is necessary when ImGui::Render() happens below.
      buildUserGuiAndInvokeCallback();

      if (options::buildGui) {
        if (options::buildDefaultGuiPanels) {
          buildPolyscopeGui();
          buildStructureGui();
          buildPickGui();
        }

        for (WeakHandle<Widget> wHandle : state::widgets) {
          if (wHandle.isValid()) {
            Widget& w = wHandle.get();
            w.buildGUI();
          }
        }
      }
    }
  }

  // Execute the context callback, if there is one.
  // This callback is a Polyscope implementation detail, which is distinct from the userCallback (which gets called
  // above)
  if (withContextCallback && contextStack.back().callback) {
    (contextStack.back().callback)();
  }

  processLazyProperties();

  // Draw structures in the scene
  if (redrawNextFrame || options::alwaysRedraw) {
    renderScene();
    redrawNextFrame = false;
  }
  renderSceneToScreen();

  // Draw the GUI
  if (withUI) {
    // render widgets
    render::engine->bindDisplay();
    for (WeakHandle<Widget> wHandle : state::widgets) {
      if (wHandle.isValid()) {
        Widget& w = wHandle.get();
        w.draw();
      }
    }

    render::engine->bindDisplay();
    render::engine->ImGuiRender();
  }
}


void mainLoopIteration() {

  processLazyProperties();
  processLazyPropertiesOutsideOfImGui();

  render::engine->makeContextCurrent();
  render::engine->updateWindowSize();

  // Process UI events
  render::engine->pollEvents();

  // Housekeeping
  purgeWidgets();

  // Rendering
  draw();
  render::engine->swapDisplayBuffers();
}

void show(size_t forFrames) {

  if (!state::initialized) {
    exception("must initialize Polyscope with polyscope::init() before calling polyscope::show().");
  }

  if (isHeadless() && forFrames == 0) {
    info("You called show() while in headless mode. In headless mode there is no display to create windows on. By "
         "default, the show() call will block indefinitely. If you did not mean to run in headless mode, check the "
         "initialization settings. Otherwise, be sure to set a callback to make something happen while polyscope is "
         "showing the UI, or use functions like screenshot() to render directly without calling show().");
  }

  unshowRequested = false;

  // the popContext() doesn't quit until _after_ the last frame, so we need to decrement by 1 to get the count right
  if (forFrames > 0) forFrames--;

  auto checkFrames = [&]() {
    if (forFrames == 0 || unshowRequested) {
      popContext();
    } else {
      forFrames--;
    }
  };

  if (options::giveFocusOnShow) {
    render::engine->focusWindow();
  }

  pushContext(checkFrames);

  if (options::usePrefsFile) {
    writePrefsFile();
  }

  // if this was the outermost show(), hide the window afterward
  if (contextStack.size() == 1) {
    if (options::hideWindowAfterShow) {
      render::engine->hideWindow();
    }
  }
}

void unshow() { unshowRequested = true; }

bool windowRequestsClose() {
  if (render::engine && render::engine->windowRequestsClose()) {
    return true;
  }

  return false;
}

bool isHeadless() {
  if (!isInitialized()) {
    exception("must initialize Polyscope with init() before calling isHeadless().");
  }
  if (render::engine) {
    return render::engine->isHeadless();
  }
  return false;
}

void shutdown(bool allowMidFrameShutdown) {

  if (!allowMidFrameShutdown && contextStack.size() > 1) {
    terminatingError("shutdown() was called mid-frame (e.g. in a per-frame callback, or UI element). This is not "
                     "permitted, shutdown() may only be called when the main loop is not executing.");
  }

  if (options::usePrefsFile) {
    writePrefsFile();
  }

  // Clear out all structures and other scene objects
  removeAllStructures();
  removeAllGroups();
  removeAllSlicePlanes();
  clearMessages();
  state::userCallback = nullptr;

  // Shut down the render engine
  render::engine->shutdown();
  delete render::engine;
  contextStack.clear();
  render::engine = nullptr;
  state::backend = "";
  state::initialized = false;
}

bool registerStructure(Structure* s, bool replaceIfPresent) {

  std::string typeName = s->typeName();
  std::map<std::string, std::unique_ptr<Structure>>& sMap = getStructureMapCreateIfNeeded(typeName);

  // Check if the structure name is in use
  bool inUse = sMap.find(s->name) != sMap.end();
  if (inUse) {
    if (replaceIfPresent) {
      removeStructure(s->name);
    } else {
      exception("Attempted to register structure with name " + s->name +
                ", but a structure with that name already exists");
      return false;
    }
  }

  // Center/scale if desired
  if (options::autocenterStructures) {
    s->centerBoundingBox();
  }
  if (options::autoscaleStructures) {
    s->rescaleToUnit();
  }

  // Add the new structure
  sMap[s->name] = std::unique_ptr<Structure>(s); // take ownership with a unique pointer
  updateStructureExtents();
  requestRedraw();

  return true;
}

Structure* getStructure(std::string type, std::string name) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    exception("No structures of type " + type + " registered");
    return nullptr;
  }
  std::map<std::string, std::unique_ptr<Structure>>& sMap = state::structures[type];

  // Special automatic case, return any
  if (name == "") {
    if (sMap.size() != 1) {
      exception("Cannot use automatic structure get with empty name unless there is exactly one structure of that type "
                "registered");
      return nullptr;
    }
    return sMap.begin()->second.get();
  }

  // General case
  if (sMap.find(name) == sMap.end()) {
    exception("No structure of type " + type + " with name " + name + " registered");
    return nullptr;
  }
  return sMap[name].get();
}

bool hasStructure(std::string type, std::string name) {
  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    return false;
  }
  std::map<std::string, std::unique_ptr<Structure>>& sMap = state::structures[type];

  // Special automatic case, return any
  if (name == "") {
    if (sMap.size() != 1) {
      exception(
          "Cannot use automatic has-structure test with empty name unless there is exactly one structure of that type "
          "registered");
    }
    return true;
  }
  return sMap.find(name) != sMap.end();
}

std::tuple<std::string, std::string> lookUpStructure(Structure* structure) {

  for (auto& typeMap : state::structures) {
    for (auto& entry : typeMap.second) {
      if (entry.second.get() == structure) {
        return std::tuple<std::string, std::string>(typeMap.first, entry.first);
      }
    }
  }

  // not found
  return std::tuple<std::string, std::string>("", "");
}

void removeStructure(std::string type, std::string name, bool errorIfAbsent) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    if (errorIfAbsent) {
      exception("No structures of type " + type + " registered");
    }
    return;
  }
  std::map<std::string, std::unique_ptr<Structure>>& sMap = state::structures[type];

  // Check if structure exists
  if (sMap.find(name) == sMap.end()) {
    if (errorIfAbsent) {
      exception("No structure of type " + type + " and name " + name + " registered");
    }
    return;
  }

  // Structure exists, remove it
  Structure* s = sMap[name].get();
  if (static_cast<void*>(s) == static_cast<void*>(internal::globalFloatingQuantityStructure)) {
    internal::globalFloatingQuantityStructure = nullptr;
  }
  // remove it from all existing groups
  for (auto& g : state::groups) {
    g.second->removeChildStructure(*s);
  }
  resetSelectionIfStructure(s);
  sMap.erase(s->name);
  updateStructureExtents();
  return;
}

void removeStructure(Structure* structure, bool errorIfAbsent) {
  removeStructure(structure->typeName(), structure->name, errorIfAbsent);
}

void removeStructure(std::string name, bool errorIfAbsent) {

  // Check if we can find exactly one structure matching the name
  Structure* targetStruct = nullptr;
  for (auto& typeMap : state::structures) {
    for (auto& entry : typeMap.second) {

      // Found a matching structure
      if (entry.first == name) {
        if (targetStruct == nullptr) {
          targetStruct = entry.second.get();
        } else {
          exception(
              "Cannot use automatic structure remove with empty name unless there is exactly one structure of that "
              "type registered. Found two structures of different types with that name: " +
              targetStruct->typeName() + " and " + typeMap.first + ".");
          return;
        }
      }
    }
  }

  // Error if none found.
  if (targetStruct == nullptr) {
    if (errorIfAbsent) {
      exception("No structure named: " + name + " to remove.");
    }
    return;
  }

  removeStructure(targetStruct->typeName(), targetStruct->name, errorIfAbsent);
  requestRedraw();
}

void removeAllStructures() {

  for (auto& typeMap : state::structures) {

    // dodge iterator invalidation
    std::vector<std::string> names;
    for (auto& entry : typeMap.second) {
      names.push_back(entry.first);
    }

    // remove all
    for (std::string name : names) {
      removeStructure(typeMap.first, name);
    }
  }

  requestRedraw();
  resetSelection();
}


Group* createGroup(std::string name) {
  checkInitialized();

  // check if group already exists
  bool inUse = state::groups.find(name) != state::groups.end();
  if (inUse) {
    exception("Attempted to register group with name " + name + ", but a group with that name already exists");
    return nullptr;
  }

  // add to the group map
  state::groups[name] = std::unique_ptr<Group>(new Group(name));

  return state::groups[name].get();
}

Group* getGroup(std::string groupName) {
  // check if group exists
  bool groupExists = state::groups.find(groupName) != state::groups.end();
  if (!groupExists) {
    exception("No group with name " + groupName + " exists");
    return nullptr;
  }

  return state::groups.find(groupName)->second.get();
}

void removeGroup(std::string name, bool errorIfAbsent) {
  // Check if group exists
  if (state::groups.find(name) == state::groups.end()) {
    if (errorIfAbsent) {
      exception("No group with name " + name + " registered");
    }
    return;
  }

  // Group exists, remove it
  state::groups.erase(name);
  return;
}

void removeGroup(Group* group, bool errorIfAbsent) { removeGroup(group->name, errorIfAbsent); }

void removeAllGroups() { state::groups.clear(); }

void refresh() {

  // reset the ground plane
  render::engine->groundPlane.prepare();

  // reset all of the structures
  for (auto& cat : state::structures) {
    for (auto& x : cat.second) {
      x.second->refresh();
    }
  }

  requestRedraw();
}

// Cached versions of lazy properties used for updates
namespace lazy {
TransparencyMode transparencyMode = TransparencyMode::None;
int transparencyRenderPasses = 8;
int ssaaFactor = 1;
float uiScale = -1.;
bool groundPlaneEnabled = true;
GroundPlaneMode groundPlaneMode = GroundPlaneMode::TileReflection;
ScaledValue<float> groundPlaneHeightFactor = 0;
int shadowBlurIters = 2;
float shadowDarkness = .4;
} // namespace lazy

void processLazyProperties() {

  // Note: This function essentially represents lazy software design, and it's an ugly and error-prone part of the
  // system. The reason for it that some settings require action on a change (e..g re-drawing the scene), but we want to
  // allow variable-set syntax like `polyscope::setting = newVal;` rather than getters and setters like
  // `polyscope::setSetting(newVal);`. It might have been better to simple set options with setter from the start, but
  // that ship has sailed.
  //
  // This function is a workaround which polls for changes to options settings, and performs any necessary additional
  // work.
  //
  // There is a second function processLazyPropertiesOutsideOfImGui() which handles a few more that can only be set
  // at limited times when an ImGui frame is not active.

  // transparency mode
  if (lazy::transparencyMode != options::transparencyMode) {
    lazy::transparencyMode = options::transparencyMode;
    render::engine->setTransparencyMode(options::transparencyMode);
  }

  // transparency render passes
  if (lazy::transparencyRenderPasses != options::transparencyRenderPasses) {
    lazy::transparencyRenderPasses = options::transparencyRenderPasses;
    requestRedraw();
  }

  // ssaa
  if (lazy::ssaaFactor != options::ssaaFactor) {
    lazy::ssaaFactor = options::ssaaFactor;
    render::engine->setSSAAFactor(options::ssaaFactor);
  }

  // ground plane
  if (lazy::groundPlaneEnabled != options::groundPlaneEnabled || lazy::groundPlaneMode != options::groundPlaneMode) {
    lazy::groundPlaneEnabled = options::groundPlaneEnabled;
    if (!options::groundPlaneEnabled) {
      // if the (depecated) groundPlaneEnabled = false, set mode to None, so we only have one variable to check
      options::groundPlaneMode = GroundPlaneMode::None;
    }
    lazy::groundPlaneMode = options::groundPlaneMode;
    requestRedraw();
  }
  if (lazy::groundPlaneHeightFactor.asAbsolute() != options::groundPlaneHeightFactor.asAbsolute() ||
      lazy::groundPlaneHeightFactor.isRelative() != options::groundPlaneHeightFactor.isRelative()) {
    lazy::groundPlaneHeightFactor = options::groundPlaneHeightFactor;
    requestRedraw();
  }
  if (lazy::shadowBlurIters != options::shadowBlurIters) {
    lazy::shadowBlurIters = options::shadowBlurIters;
    requestRedraw();
  }
  if (lazy::shadowDarkness != options::shadowDarkness) {
    lazy::shadowDarkness = options::shadowDarkness;
    requestRedraw();
  }
};

void processLazyPropertiesOutsideOfImGui() {
  // Like processLazyProperties, but this one handles properties which cannot be changed mid-ImGui frame

  // uiScale
  if (lazy::uiScale != options::uiScale) {
    lazy::uiScale = options::uiScale;
    render::engine->configureImGui();
    setInitialWindowWidths();
  }
}

void updateStructureExtents() {

  if (!options::automaticallyComputeSceneExtents) {
    return;
  }

  // Note: the cost multiple calls to this function scales only with the number of structures, not the size of the data
  // in those structures, because structures internally cache the extents of their data.

  // Compute length scale and bbox as the max of all structures
  state::lengthScale = 0.0;
  glm::vec3 minBbox = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 maxBbox = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (auto& cat : state::structures) {
    for (auto& x : cat.second) {
      if (!x.second->hasExtents()) {
        continue;
      }
      state::lengthScale = std::max(state::lengthScale, x.second->lengthScale());
      auto bbox = x.second->boundingBox();
      minBbox = componentwiseMin(minBbox, std::get<0>(bbox));
      maxBbox = componentwiseMax(maxBbox, std::get<1>(bbox));
    }
  }

  // If we got a non-finite bounding box, fix it
  if (!isFinite(minBbox) || !isFinite(maxBbox)) {
    minBbox = -glm::vec3{1, 1, 1};
    maxBbox = glm::vec3{1, 1, 1};
  }

  // If we got a degenerate bounding box, perturb it slightly
  if (minBbox == maxBbox) {
    double offsetScale = (state::lengthScale == 0) ? 1e-5 : state::lengthScale * 1e-5;
    glm::vec3 offset{offsetScale, offsetScale, offsetScale};
    minBbox = minBbox - offset / 2.f;
    maxBbox = maxBbox + offset / 2.f;
  }

  std::get<0>(state::boundingBox) = minBbox;
  std::get<1>(state::boundingBox) = maxBbox;

  // If we got a bounding box but not a length scale we can use the size of the
  // box as a scale. If we got neither, we'll end up with a constant near 1 due
  // to the above correction
  if (state::lengthScale == 0) {
    state::lengthScale = glm::length(maxBbox - minBbox);
  }

  requestRedraw();
}

namespace state {
glm::vec3 center() { return 0.5f * (std::get<0>(state::boundingBox) + std::get<1>(state::boundingBox)); }
} // namespace state

} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/polyscope.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "imgui.h"

#include "polyscope/pick.h"
#include "polyscope/render/engine.h"
#include "polyscope/view.h"

#include "stb_image.h"

#include "json/json.hpp"
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
  std::function<void()> callback;
  bool drawDefaultUI;
};
std::vector<ContextEntry> contextStack;

bool redrawNextFrame = true;

// Some state about imgui windows to stack them
float imguiStackMargin = 10;
float lastWindowHeightPolyscope = 200;
float lastWindowHeightUser = 200;
float leftWindowsWidth = 305;
float rightWindowsWidth = 500;


const std::string prefsFilename = ".polyscope.ini";

void readPrefsFile() {

  try {

    std::ifstream inStream(prefsFilename);
    if (inStream) {

      json prefsJSON;
      inStream >> prefsJSON;

      // Set values
      if (prefsJSON.count("windowWidth") > 0) {
        view::windowWidth = prefsJSON["windowWidth"];
      }
      if (prefsJSON.count("windowHeight") > 0) {
        view::windowHeight = prefsJSON["windowHeight"];
      }
      if (prefsJSON.count("windowPosX") > 0) {
        view::initWindowPosX = prefsJSON["windowPosX"];
      }
      if (prefsJSON.count("windowPosY") > 0) {
        view::initWindowPosY = prefsJSON["windowPosY"];
      }
    }

  }
  // We never really care if something goes wrong while loading preferences, so eat all exceptions
  catch (...) {
    polyscope::warning("Parsing of prefs file failed");
  }
}

void writePrefsFile() {

  // Update values as needed
  int posX, posY;
  std::tie(posX, posY) = render::engine->getWindowPos();

  // Build json object
  json prefsJSON = {
      {"windowWidth", view::windowWidth},
      {"windowHeight", view::windowHeight},
      {"windowPosX", posX},
      {"windowPosY", posY},
  };

  // Write out json object
  std::ofstream o(prefsFilename);
  o << std::setw(4) << prefsJSON << std::endl;
}

}; // namespace

// === Core global functions

void init(std::string backend) {
  if (state::initialized) {
    if (backend != state::backend) {
      throw std::runtime_error("re-initializing with different backend is not supported");
    }
    // otherwise silently allow multiple-init
    return;
  }

  state::backend = backend;

  if (options::usePrefsFile) {
    readPrefsFile();
  }

  // Initialize the rendering engine
  render::initializeRenderEngine(backend);

  // Initialie ImGUI
  IMGUI_CHECKVERSION();
  render::engine->initializeImGui();
  // push a fake context which will never be used (but dodges some invalidation issues)
  contextStack.push_back(ContextEntry{ImGui::GetCurrentContext(), nullptr, false});

  view::invalidateView();

  state::initialized = true;
}

void pushContext(std::function<void()> callbackFunction, bool drawDefaultUI) {

  // Create a new context and push it on to the stack
  ImGuiContext* newContext = ImGui::CreateContext(render::engine->getImGuiGlobalFontAtlas());
  ImGuiIO& oldIO = ImGui::GetIO(); // used to copy below, see note
  ImGui::SetCurrentContext(newContext);

  render::engine->setImGuiStyle();
  ImGui::GetIO() = oldIO; // Copy all of the old IO values to new. With ImGUI 1.76 (and some previous versions), this
                          // was necessary to fix a bug where keys like delete, etc would break in subcontexts. The
                          // problem was that the key mappings (e.g. GLFW_KEY_BACKSPACE --> ImGuiKey_Backspace) need to
                          // be populated in io.KeyMap, and these entries would get lost on creating a new context.
  contextStack.push_back(ContextEntry{newContext, callbackFunction, drawDefaultUI});

  if (contextStack.size() > 50) {
    // Catch bugs with nested show()
    throw std::runtime_error(
        "Uh oh, polyscope::show() was recusively MANY times (depth > 50), this is probably a bug. Perhaps "
        "you are accidentally calling show every time polyscope::userCallback executes?");
  };

  // Make sure the window is visible
  render::engine->showWindow();

  // Re-enter main loop until the context has been popped
  size_t currentContextStackSize = contextStack.size();
  while (contextStack.size() >= currentContextStackSize) {

    mainLoopIteration();

    // auto-exit if the window is closed
    if (render::engine->windowRequestsClose()) {
      popContext();
    }
  }

  oldIO = ImGui::GetIO(); // Copy new IO values to old. I haven't encountered anything that strictly requires this, but
                          // it feels like we should mirror the behavior from pushing.

  ImGui::DestroyContext(newContext);

  // Restore the previous context, if there was one
  if (!contextStack.empty()) {
    ImGui::SetCurrentContext(contextStack.back().context);
  }
}


void popContext() {
  if (contextStack.empty()) {
    error("Called popContext() too many times");
    return;
  }
  contextStack.pop_back();
}

void requestRedraw() { redrawNextFrame = true; }
bool redrawRequested() { return redrawNextFrame; }

void drawStructures() {

  // Draw all off the structures registered with polyscope

  for (auto catMap : state::structures) {
    for (auto s : catMap.second) {
      // make sure the right settings are active
      // render::engine->setDepthMode();
      // render::engine->applyTransparencySettings();

      s.second->draw();
    }
  }
}


namespace {

float dragDistSinceLastRelease = 0.0;

void processInputEvents() {
  ImGuiIO& io = ImGui::GetIO();

  // If any mouse button is pressed, trigger a redraw
  if (ImGui::IsAnyMouseDown()) {
    requestRedraw();
  }

  bool widgetCapturedMouse = false;
  for (Widget* w : state::widgets) {
    widgetCapturedMouse = w->interact();
    if (widgetCapturedMouse) {
      break;
    }
  }

  // Handle scroll events for 3D view
  if (!io.WantCaptureMouse && !widgetCapturedMouse) {
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
        bool scrollClipPlane = io.KeyShift;

        if (scrollClipPlane) {
          view::processClipPlaneShift(maxScroll);
        } else {
          view::processZoom(maxScroll);
        }
      }
    }
  }

  // === Mouse inputs
  if (!io.WantCaptureMouse && !widgetCapturedMouse) {

    // Process drags
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
        view::processZoom(dragDelta.y * 5);
      }
      if (isRotate) {
        glm::vec2 currPos{io.MousePos.x / view::windowWidth, (view::windowHeight - io.MousePos.y) / view::windowHeight};
        currPos = (currPos * 2.0f) - glm::vec2{1.0, 1.0};
        if (std::abs(currPos.x) <= 1.0 && std::abs(currPos.y) <= 1.0) {
          view::processRotate(currPos - 2.0f * dragDelta, currPos);
        }
      }
      if (isTranslate) {
        view::processTranslate(dragDelta);
      }
    }

    // Click picks
    float dragIgnoreThreshold = 0.01;
    if (ImGui::IsMouseReleased(0)) {

      // Don't pick at the end of a long drag
      if (dragDistSinceLastRelease < dragIgnoreThreshold) {
        ImVec2 p = ImGui::GetMousePos();
        std::pair<Structure*, size_t> pickResult =
            pick::evaluatePickQuery(io.DisplayFramebufferScale.x * p.x, io.DisplayFramebufferScale.y * p.y);
        pick::setSelection(pickResult);
      }

      // Reset the drag distance after any release
      dragDistSinceLastRelease = 0.0;
    }
    // Clear pick
    if (ImGui::IsMouseReleased(1)) {
      if (dragDistSinceLastRelease < dragIgnoreThreshold) {
        pick::resetSelection();
      }
      dragDistSinceLastRelease = 0.0;
    }
  }

  // === Key-press inputs
  if (!io.WantCaptureKeyboard) {

    // ctrl-c
    if (io.KeyCtrl && render::engine->isKeyPressed('c')) {
      std::string outData = view::getCameraJson();
      render::engine->setClipboardText(outData);
    }

    // ctrl-v
    if (io.KeyCtrl && render::engine->isKeyPressed('v')) {
      std::string clipboardData = render::engine->getClipboardText();
      view::setCameraFromJson(clipboardData, true);
    }
  }
}

void renderSlicePlanes() {
  for (SlicePlane* s : state::slicePlanes) {
    s->draw();
  }
}

void renderScene() {
  processLazyProperties();

  render::engine->applyTransparencySettings();

  render::engine->sceneBuffer->clearColor = {0., 0., 0.};
  render::engine->sceneBuffer->clearAlpha = 0.;
  render::engine->sceneBuffer->clear();

  if (!render::engine->bindSceneBuffer()) return;

  // If a view has never been set, this will set it to the home view
  view::ensureViewValid();

  if (render::engine->getTransparencyMode() == TransparencyMode::Pretty) {
    // Special depth peeling case: multiple render passes
    // We will perform several "peeled" rounds of rendering in to the usual scene buffer. After each, we will manually
    // composite in to the final scene buffer.


    // Clear the final buffer explicitly since we will gradually composite in to it rather than just blitting directly
    // as in normal rendering.
    render::engine->sceneBufferFinal->clearColor = glm::vec3{0., 0., 0.};
    render::engine->sceneBufferFinal->clearAlpha = 0;
    render::engine->sceneBufferFinal->clear();

    render::engine->setDepthMode(); // we need depth to be enabled for the clear below to do anything
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
      }

      // Composite the result of this pass in to the result buffer
      render::engine->sceneBufferFinal->bind();
      render::engine->setDepthMode(DepthMode::Disable);
      render::engine->setBlendMode(BlendMode::Under);
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

    render::engine->sceneBuffer->blitTo(render::engine->sceneBufferFinal.get());
  }
} // namespace

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

void buildPolyscopeGui() {

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
    screenshot(options::screenshotTransparency);
  }
  ImGui::SameLine();
  if (ImGui::ArrowButton("##Option", ImGuiDir_Down)) {
    ImGui::OpenPopup("ScreenshotOptionsPopup");
  }
  ImGui::PopStyleVar();
  if (ImGui::BeginPopup("ScreenshotOptionsPopup")) {

    ImGui::Checkbox("with transparency", &options::screenshotTransparency);

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
			ImGui::TextUnformatted("      Rotate: [left click drag]");
			ImGui::TextUnformatted("   Translate: [shift] + [left click drag] OR [right click drag]");
			ImGui::TextUnformatted("        Zoom: [scroll] OR [ctrl] + [shift] + [left click drag]");
			ImGui::TextUnformatted("   Use [ctrl-c] and [ctrl-v] to save and restore camera poses");
			ImGui::TextUnformatted("     via the clipboard.");
		ImGui::TextUnformatted("\nMenu Navigation:");			
			ImGui::TextUnformatted("   Menu headers with a '>' can be clicked to collapse and expand.");
			ImGui::TextUnformatted("   Use [ctrl] + [left click] to manually enter any numeric value");
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

  // Debug options tree
  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
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

  // fps
  ImGui::Text("%.1f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

  lastWindowHeightPolyscope = imguiStackMargin + ImGui::GetWindowHeight();
  leftWindowsWidth = ImGui::GetWindowWidth();

  ImGui::End();
}

void buildStructureGui() {
  // Create window
  static bool showStructureWindow = true;

  ImGui::SetNextWindowPos(ImVec2(imguiStackMargin, lastWindowHeightPolyscope + 2 * imguiStackMargin));
  ImGui::SetNextWindowSize(
      ImVec2(leftWindowsWidth, view::windowHeight - lastWindowHeightPolyscope - 3 * imguiStackMargin));

  ImGui::Begin("Structures", &showStructureWindow);

  for (auto catMapEntry : state::structures) {
    std::string catName = catMapEntry.first;

    std::map<std::string, Structure*>& structureMap = catMapEntry.second;

    ImGui::PushID(catName.c_str()); // ensure there are no conflicts with
                                    // identically-named labels

    // Build the structure's UI
    ImGui::SetNextTreeNodeOpen(structureMap.size() > 0, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader((catName + " (" + std::to_string(structureMap.size()) + ")").c_str())) {
      // Draw shared GUI elements for all instances of the structure
      if (structureMap.size() > 0) {
        structureMap.begin()->second->buildSharedStructureUI();
      }

      for (auto x : structureMap) {
        ImGui::SetNextTreeNodeOpen(structureMap.size() <= 8,
                                   ImGuiCond_FirstUseEver); // closed by default if more than 8
        x.second->buildUI();
      }
    }

    ImGui::PopID();
  }

  leftWindowsWidth = ImGui::GetWindowWidth();

  ImGui::End();
}

void buildUserGuiAndInvokeCallback() {

  if (!options::invokeUserCallbackForNestedShow && contextStack.size() > 2) {
    return;
  }

  if (state::userCallback) {
    ImGui::PushID("user_callback");

    if (options::openImGuiWindowForUserCallback) {
      ImGui::SetNextWindowPos(ImVec2(view::windowWidth - (rightWindowsWidth + imguiStackMargin), imguiStackMargin));
      ImGui::SetNextWindowSize(ImVec2(rightWindowsWidth, 0.));

      ImGui::Begin("Command UI", nullptr);
    }

    state::userCallback();

    if (options::openImGuiWindowForUserCallback) {
      rightWindowsWidth = ImGui::GetWindowWidth();
      lastWindowHeightUser = imguiStackMargin + ImGui::GetWindowHeight();
      ImGui::End();
    } else {
      lastWindowHeightUser = imguiStackMargin;
    }

    ImGui::PopID();
  } else {
    lastWindowHeightUser = imguiStackMargin;
  }
}

void buildPickGui() {
  if (pick::haveSelection()) {

    ImGui::SetNextWindowPos(ImVec2(view::windowWidth - (rightWindowsWidth + imguiStackMargin),
                                   2 * imguiStackMargin + lastWindowHeightUser));
    ImGui::SetNextWindowSize(ImVec2(rightWindowsWidth, 0.));

    ImGui::Begin("Selection", nullptr);
    std::pair<Structure*, size_t> selection = pick::getSelection();

    ImGui::TextUnformatted((selection.first->typeName() + ": " + selection.first->name).c_str());
    ImGui::Separator();
    selection.first->buildPickUI(selection.second);

    rightWindowsWidth = ImGui::GetWindowWidth();
    ImGui::End();
  }
}

auto lastMainLoopIterTime = std::chrono::steady_clock::now();

} // namespace

void draw(bool withUI) {
  processLazyProperties();

  // Update buffer and context
  render::engine->makeContextCurrent();
  render::engine->bindDisplay();
  render::engine->setBackgroundColor({view::bgColor[0], view::bgColor[1], view::bgColor[2]});
  render::engine->setBackgroundAlpha(view::bgColor[3]);
  render::engine->clearDisplay();

  if (withUI) {
    render::engine->ImGuiNewFrame();
  }

  // Build the GUI components
  if (withUI) {
    if (contextStack.back().drawDefaultUI) {

      // Note: It is important to build the user GUI first, because it is likely that callbacks there will modify
      // polyscope data. If we do these modifications happen later in the render cycle, they might invalidate data which
      // is necessary when ImGui::Render() happens below.
      buildUserGuiAndInvokeCallback();

      buildPolyscopeGui();
      buildStructureGui();
      buildPickGui();

      for (Widget* w : state::widgets) {
        w->buildGUI();
      }
    }
  }

  // Execute the context callback, if there is one.
  // This callback is Polyscope implementation detail, which is distinct from the userCallback (which gets called below)
  if (contextStack.back().callback) {
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
    for (Widget* w : state::widgets) {
      w->draw();
    }

    render::engine->bindDisplay();
    render::engine->ImGuiRender();
  }
}


void mainLoopIteration() {

  processLazyProperties();

  // The windowing system will let this busy-loop in some situations, unfortunately. Make sure that doesn't happen.
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

  render::engine->makeContextCurrent();
  render::engine->updateWindowSize();

  // Process UI events
  render::engine->pollEvents();
  processInputEvents();
  view::updateFlight();
  showDelayedWarnings();

  // Rendering
  draw();
  render::engine->swapDisplayBuffers();
}

void show(size_t forFrames) {

  if (!state::initialized) {
    throw std::logic_error(options::printPrefix +
                           "must initialize Polyscope with polyscope::init() before calling polyscope::show().");
  }

  auto checkFrames = [&]() {
    if (forFrames == 0) {
      popContext();
    } else {
      forFrames--;
    }
  };
  pushContext(checkFrames);

  if (options::usePrefsFile) {
    writePrefsFile();
  }

  // if this was the outermost show(), hide the window afterward
  if (contextStack.size() == 1) {
    render::engine->hideWindow();
  }
}

void shutdown(int exitCode) {

  // TODO should we make an effort to destruct everything here?
  if (options::usePrefsFile) {
    writePrefsFile();
  }

  render::engine->shutdownImGui();

  std::exit(exitCode);
}

bool registerStructure(Structure* s, bool replaceIfPresent) {

  // Make sure a map for the type exists
  std::string typeName = s->typeName();
  if (state::structures.find(typeName) == state::structures.end()) {
    state::structures[typeName] = std::map<std::string, Structure*>();
  }
  std::map<std::string, Structure*>& sMap = state::structures[typeName];

  // Check if the structure name is in use
  bool inUse = sMap.find(s->name) != sMap.end();
  if (inUse) {
    if (replaceIfPresent) {
      removeStructure(s->name);
    } else {
      polyscope::error("Attempted to register structure with name " + s->name +
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
  sMap[s->name] = s;
  updateStructureExtents();
  requestRedraw();

  return true;
}

Structure* getStructure(std::string type, std::string name) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    error("No structures of type " + type + " registered");
    return nullptr;
  }
  std::map<std::string, Structure*>& sMap = state::structures[type];

  // Special automatic case, return any
  if (name == "") {
    if (sMap.size() != 1) {
      error("Cannot use automatic structure get with empty name unless there is exactly one structure of that type "
            "registered");
      return nullptr;
    }
    return sMap.begin()->second;
  }

  // General case
  if (sMap.find(name) == sMap.end()) {
    error("No structure of type " + type + " with name " + name + " registered");
    return nullptr;
  }
  return sMap[name];
}

bool hasStructure(std::string type, std::string name) {
  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    return false;
  }
  std::map<std::string, Structure*>& sMap = state::structures[type];

  // Special automatic case, return any
  if (name == "") {
    if (sMap.size() != 1) {
      error("Cannot use automatic structure get with empty name unless there is exactly one structure of that type "
            "registered");
    }
    return true;
  }
  return sMap.find(name) != sMap.end();
}


void removeStructure(std::string type, std::string name, bool errorIfAbsent) {

  // If there are no structures of that type it is an automatic fail
  if (state::structures.find(type) == state::structures.end()) {
    if (errorIfAbsent) {
      error("No structures of type " + type + " registered");
    }
    return;
  }
  std::map<std::string, Structure*>& sMap = state::structures[type];

  // Check if structure exists
  if (sMap.find(name) == sMap.end()) {
    if (errorIfAbsent) {
      error("No structure of type " + type + " and name " + name + " registered");
    }
    return;
  }

  // Structure exists, remove it
  Structure* s = sMap[name];
  pick::resetSelectionIfStructure(s);
  sMap.erase(s->name);
  delete s;
  updateStructureExtents();
  return;
}

void removeStructure(Structure* structure, bool errorIfAbsent) {
  removeStructure(structure->typeName(), structure->name, errorIfAbsent);
}

void removeStructure(std::string name, bool errorIfAbsent) {

  // Check if we can find exactly one structure matching the name
  Structure* targetStruct = nullptr;
  for (auto typeMap : state::structures) {
    for (auto entry : typeMap.second) {

      // Found a matching structure
      if (entry.first == name) {
        if (targetStruct == nullptr) {
          targetStruct = entry.second;
        } else {
          error("Cannot use automatic structure remove with empty name unless there is exactly one structure of that "
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
      error("No structure named: " + name + " to remove.");
    }
    return;
  }

  removeStructure(targetStruct->typeName(), targetStruct->name, errorIfAbsent);
  requestRedraw();
}

void removeAllStructures() {

  for (auto typeMap : state::structures) {

    // dodge iterator invalidation
    std::vector<std::string> names;
    for (auto entry : typeMap.second) {
      names.push_back(entry.first);
    }

    // remove all
    for (auto name : names) {
      removeStructure(typeMap.first, name);
    }
  }

  requestRedraw();
  pick::resetSelection();
}

void refresh() {

  // reset the ground plane
  render::engine->groundPlane.prepare();

  // reset all of the structures
  for (auto cat : state::structures) {
    for (auto x : cat.second) {
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
    render::engine->groundPlane.prepare();
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

void updateStructureExtents() {
  // Compute length scale and bbox as the max of all structures
  state::lengthScale = 0.0;
  glm::vec3 minBbox = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 maxBbox = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (auto cat : state::structures) {
    for (auto x : cat.second) {
      state::lengthScale = std::max(state::lengthScale, x.second->lengthScale());
      auto bbox = x.second->boundingBox();
      minBbox = componentwiseMin(minBbox, std::get<0>(bbox));
      maxBbox = componentwiseMax(maxBbox, std::get<1>(bbox));
    }
  }

  if (!isFinite(minBbox) || !isFinite(maxBbox)) {
    minBbox = -glm::vec3{1, 1, 1};
    maxBbox = glm::vec3{1, 1, 1};
  }
  std::get<0>(state::boundingBox) = minBbox;
  std::get<1>(state::boundingBox) = maxBbox;

  // If we got a bounding box but not a length scale we can use the size of the
  // box as a scale. If we got neither, we'll end up with a constant near 1 due
  // to the above correction
  if (state::lengthScale == 0) {
    state::lengthScale = glm::length(maxBbox - minBbox);
  }

  // Center is center of bounding box
  state::center = 0.5f * (minBbox + maxBbox);
}


} // namespace polyscope

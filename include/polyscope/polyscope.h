// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <functional>
#include <map>
#include <unordered_set>

#include "polyscope/gl/gl_utils.h"
#include "polyscope/messages.h"
#include "polyscope/options.h"
#include "polyscope/screenshot.h"
#include "polyscope/structure.h"
#include "polyscope/utilities.h"

#include "imgui.h"

namespace polyscope {

// Initialize polyscope, including windowing system and openGL. Should be
// called exactly once at the beginning of a program. If initialization
// fails in any way, an exception will be thrown.
void init();

// Give control to the polyscope GUI. Blocks until the user returns control via
// the GUI, possibly by exiting the window.
void show();

// Do shutdown work and quit the entire program. Can be called in other situations due to errors (etc)
void shutdown(int exitCode = 0);

// === Global variables ===
namespace state {

// has polyscope::init() been called?
extern bool initialized;

// lists of all structures in Polyscope, by category
extern std::map<std::string, std::map<std::string, Structure*>> structures;

// representative length scale for all registered structures
extern double lengthScale;

// axis-aligned bounding box for all registered structures
extern std::tuple<glm::vec3, glm::vec3> boundingBox;

// representative center for all registered structures
extern glm::vec3 center;

// A callback function used to render a "user" gui
extern std::function<void()> userCallback;

} // namespace state

// === Manage structures tracked by polyscope

// Register a structure with polyscope
// Structure name must be a globally unique identifier for the structure.
bool registerStructure(Structure* structure, bool replaceIfPresent = true);

// Get a reference to a structure that has been registered
// The default version with name="" arbitrarily returns any structure of that type. This is useful as a shorthand when
// only using a single structure.
Structure* getStructure(std::string type, std::string name = "");

// De-register a structure, of any type. Also removes any quantities associated with the structure
void removeStructure(Structure* structure, bool errorIfAbsent = true);
void removeStructure(std::string type, std::string name, bool errorIfAbsent = true);
void removeStructure(std::string name, bool errorIfAbsent = true);

// De-register all structures, of any type. Also removes any quantities associated with the structure
void removeAllStructures();

// Recompute the global state::lengthScale, boundingBox, and center by looping over registered structures
void updateStructureExtents();

// === Handle draw flow, interrupts, and popups

// Actually render to the framebuffer.
void draw(bool withUI = true);

// Request that the 3D scene be redrawn for the next frame. Should be called anytime something changes in the scene.
void requestRedraw();

// Has a redraw been requested for the next frame?
bool redrawRequested();

// A callback function currently drawing a focused popup GUI
// If this is non-null, then we should not draw or respect any IMGUI elements except those drawn within this function.
void pushContext(std::function<void()> callbackFunction);
void popContext();

// === Utility

// Execute one iteration of the main loop
// Exposed so that some weird flow (eg, errors) can re-enter the main loop when appropriate. Be careful!
void mainLoopIteration();
void bindDefaultBuffer();
void initializeImGUIContext();
void drawStructures();

// Share a font atlas for multiple uses (mainly with imgui)
ImFontAtlas* getGlobalFontAtlas();

} // namespace polyscope

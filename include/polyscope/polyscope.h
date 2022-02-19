// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/internal.h"
#include "polyscope/messages.h"
#include "polyscope/options.h"
#include "polyscope/screenshot.h"
#include "polyscope/slice_plane.h"
#include "polyscope/structure.h"
#include "polyscope/utilities.h"
#include "polyscope/widget.h"
#include "polyscope/transformation_gizmo.h"
#include "imgui.h"

#include <functional>
#include <map>
#include <set>
#include <unordered_set>


namespace polyscope {

// forward declarations
class Structure;

// Initialize polyscope, including windowing system and openGL. Should be called exactly once at the beginning of a
// program. If initialization fails in any way, an exception will be thrown.
// The backend string sets which rendering backend to use. If "", a reasonable default backend will be chosen.
void init(std::string backend = "");

// Give control to the polyscope GUI. Blocks until the user returns control via
// the GUI, possibly by exiting the window.
void show(size_t forFrames = std::numeric_limits<size_t>::max());

// Do shutdown work and de-initialize Polyscope
void shutdown();

// === Global variables ===
namespace state {

// has polyscope::init() been called?
extern bool initialized;

// what backend was set on initialization
extern std::string backend;

// lists of all structures in Polyscope, by category
extern std::map<std::string, std::map<std::string, Structure*>> structures;

// representative length scale for all registered structures
extern float lengthScale;

// axis-aligned bounding box for all registered structures
extern std::tuple<glm::vec3, glm::vec3> boundingBox;

// a list of widgets and other more specific doodads in the scene
extern std::set<Widget*> widgets;
extern std::vector<SlicePlane*> slicePlanes;

// should we allow default trackball mouse camera interaction? 
// Needs more interactions on when to turn this on/off
extern bool doDefaultMouseInteraction;

// a callback function used to render a "user" gui
extern std::function<void()> userCallback;




// representative center for all registered structures
glm::vec3 center();

} // namespace state

// === Manage structures tracked by polyscope

// Register a structure with polyscope
// Structure name must be a globally unique identifier for the structure.
bool registerStructure(Structure* structure, bool replaceIfPresent = true);

// Get a reference to a structure that has been registered
// The default version with name="" arbitrarily returns any structure of that type. This is useful as a shorthand when
// only using a single structure.
Structure* getStructure(std::string type, std::string name = "");

// True if such a structure exists
bool hasStructure(std::string type, std::string name = "");

// De-register a structure, of any type. Also removes any quantities associated with the structure
void removeStructure(Structure* structure, bool errorIfAbsent = true);
void removeStructure(std::string type, std::string name, bool errorIfAbsent = true);
void removeStructure(std::string name, bool errorIfAbsent = true);

// De-register all structures, of any type. Also removes any quantities associated with the structure
void removeAllStructures();

// Recompute the global state::lengthScale, boundingBox, and center by looping over registered structures
void updateStructureExtents();

// Essentially regenerates all state and programs within Polyscope, calling refresh() recurisvely on all structures and
// quantities
void refresh();

// === Handle draw flow, interrupts, and popups

// Main draw call, which handles all 3D rendering & UI management.
// End users generally should not call this function. Consider requestRedraw() or screenshot().
void draw(bool withUI = true, bool withContextCallback = true);

// Request that the 3D scene be redrawn for the next frame. Should be called anytime something changes in the scene.
void requestRedraw();

// Has a redraw been requested for the next frame?
bool redrawRequested();

// Managed a stack of of contexts to draw the UI. Usually contains one entry, which causes the main GUI to be drawn, but
// in general the top callback will be called instead. Primarily exists to manage the ImGUI context, so callbacks can
// create other contexts and circumvent the main draw loop. This is used internally to implement messages, element
// selections, etc.
void pushContext(std::function<void()> callbackFunction, bool drawDefaultUI = true);
void popContext();

// These helpers are called internally by Polyscope to render and build the UI.
// Normally, applications should not need to call them, but in advanced settings when making custom UIs, they may be
// useful to manually build pieces of the interface.
void buildPolyscopeGui();
void buildStructureGui();
void buildPickGui();
void buildUserGuiAndInvokeCallback();


// === Utility

// Execute one iteration of the main loop
// Exposed so that some weird flow (eg, errors) can re-enter the main loop when appropriate. Be careful!
void mainLoopIteration();
void initializeImGUIContext();
void drawStructures();

// Called to check any options that might have been changed and perform appropriate updates. Users generally should not
// need to call this directly.
void processLazyProperties();


} // namespace polyscope

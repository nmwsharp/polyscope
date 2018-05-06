#pragma once

#include <map>
#include <unordered_set>

#include "polyscope/camera_view.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/messages.h"
#include "polyscope/options.h"
#include "polyscope/point_cloud.h"
#include "polyscope/ray_set.h"
#include "polyscope/structure.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/utilities.h"

#include "imgui.h"

namespace polyscope {

// Initialize polyscope, including windowing system and openGL. Should be
// called exactly once at the beginning of a program. If initialization
// fails in any way, an exception will be thrown.
void init();

// Give control to the polyscope GUI. Blocks until the user returns control via
// the GUI, possibly by exiting the window.
void show(bool shutdownAfter = true);

// Do shutdown work and quit. Usually called at end of main loop, can be called in other situations due to errors (etc)
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
extern std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> boundingBox;

// representative center for all registered structures
extern Vector3 center;

// A callback function used to render a "user" gui
extern std::function<void()> userCallback;

// The current screenshot index for automatically numbered screenshots
extern size_t screenshotInd;

} // namespace state

// === Manage structures tracked by polyscope

// Register a structure with polyscope
// `name` is a globally unique identifier for the structure
bool registerStructure(Structure* structure, bool replaceIfPresent = true);
void registerPointCloud(std::string name, const std::vector<Vector3>& points, bool replaceIfPresent = true);
void registerSurfaceMesh(std::string name, Geometry<Euclidean>* geom, bool replaceIfPresent = true);
void registerCameraView(std::string name, CameraParameters p, bool replaceIfPresent = true);
void registerRaySet(std::string name, const std::vector<std::vector<RayPoint>>& r, bool replaceIfPresent = true);

// Get a reference to a structure that has been registered
// The default version with name="" arbitrarily returns any structure of that type. This is useful as a shorthand when
// only using a single structure.
Structure* getStructure(std::string type, std::string name = "");
PointCloud* getPointCloud(std::string name = "");
SurfaceMesh* getSurfaceMesh(std::string name = "");
CameraView* getCameraView(std::string name = "");
RaySet* getRaySet(std::string name = "");

// De-register a structure, of any type. Also removes any quantities associated with the structure
void removeStructure(std::string type, std::string name, bool errorIfAbsent = true);
void removeStructure(std::string name);

// De-register all structures, of any type. Also removes any quantities associated with the structure
void removeAllStructures();

// Recompute state::lengthScale, boundingBox, and center from all registered structures
void updateStructureExtents();

// === Handle interrupts and popups

// A callback function currently drawing a focused popup GUI
// If this is non-null, then we should not draw or respect any IMGUI elements except those drawn within this function.
// Note that there is currently no notion of recursive popups, so the programmer should be careful about invoking
// another popup within this function.
extern std::function<void()> focusedPopupUI;

// === Utility

// Execute one iteration of the main loop
// Exposed so that some weird flow (eg, errors) can re-enter the main loop when appropriate. Be careful!
void mainLoopIteration();
void bindDefaultBuffer();
void initializeImGUIContext();

// Take screenshots of the current view
void screenshot(std::string filename, bool transparentBG = true);
void screenshot(bool transparentBG = true);
void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels);

// Share a font atlas for multiple uses (mainly with imgui)
ImFontAtlas* getGlobalFontAtlas();

} // namespace polyscope

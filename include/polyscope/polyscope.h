#pragma once

#include <unordered_map>
#include <unordered_set>

#include "polyscope/gl/gl_utils.h"
#include "polyscope/options.h"
#include "polyscope/point_cloud.h"

namespace polyscope {

// Initialize polyscope, including windowing system and openGL. Should be
// called exactly once at the beginning of a program. If initialization
// fails in any way, an exception will be thrown.
void init();

// Give control to the polyscope GUI. Blocks until the user returns control via
// the GUI, possibly by exiting the window.
void show();

// === Global variables ===
namespace state {

// has polyscope::init() been called?
extern bool initialized;

// lists of all structures in polyscope
extern std::unordered_set<std::string> allStructureNames;
extern std::unordered_map<std::string, PointCloud*> pointClouds;

}  // namespace state

// === Manage structures tracked by polyscope

// Register a point cloud structure with polyscope
// `name` is a globally unique identifier for the structure
void registerPointCloud(std::string name, const std::vector<Vector3>& points);

// De-register a structure, of any type. Also removes any quantities associated with the structure
void removeStructure(std::string name);

// De-register all structures, of any type. Also removes any quantities associated with the structure
void removeAllStructures();


// === Errors
void error(std::string message);

}  // namespace polyscope
#pragma once

#include <map>
#include <unordered_set>

#include "polyscope/gl/gl_utils.h"
#include "polyscope/options.h"
#include "polyscope/point_cloud.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/structure.h"

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

// lists of all structures in polyscope (used to itearte over all structures)
extern std::map<StructureType, std::map<std::string, Structure*>> structureCategories;
// also lists all structues, but lists are typed 
extern std::map<std::string, PointCloud*> pointClouds;
extern std::map<std::string, SurfaceMesh*> surfaceMeshes;

// representative length scale for all registered structures
extern double lengthScale;

// axis-alligned bounding box for all registered structures
extern std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> boundingBox;

// representative center for all registered structures
extern Vector3 center;

}  // namespace state

// === Manage structures tracked by polyscope

// Register a point cloud structure with polyscope
// `name` is a globally unique identifier for the structure
void registerPointCloud(std::string name, const std::vector<Vector3>& points);
void registerSurfaceMesh(std::string name, Geometry<Euclidean>* geom);

// De-register a structure, of any type. Also removes any quantities associated with the structure
void removeStructure(std::string name);

// De-register all structures, of any type. Also removes any quantities associated with the structure
void removeAllStructures();

// Recompute state::lengthScale, boundingBox, and center from all registered structures 
void updateStructureExtents();

// === Errors
void error(std::string message);

}  // namespace polyscope
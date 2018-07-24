#pragma once

#include <vector>

#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/structure.h"

namespace polyscope {

struct RayPoint {
  RayPoint(glm::vec3 v_, bool isInf = false) : v(v_), isInfiniteDirection(isInf) {}

  glm::vec3 v;

  // if true, rather than being a point on a path this is a direction, along
  // which the ray heads to infinity
  bool isInfiniteDirection;
};

class RaySet : public Structure {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  RaySet(std::string name, const std::vector<std::vector<RayPoint>>& r);
  ~RaySet();

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;
  virtual void preparePick() override;

  // Build the imgui display
  virtual void drawUI() override;
  virtual void drawPickUI(size_t localPickID) override;
  virtual void drawSharedStructureUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;

  bool enabled = false;

  static const std::string structureTypeName;

private:
  // The ray paths in the set
  std::vector<std::vector<RayPoint>> rayPaths;

  // Visualization parameters
  Color3f rayColor;
  Color3f baseColor;
  SubColorManager colorManager;
  float viewIntervalFactor = 1.0;
  float streakLengthFactor = 0.05;
  float speedFactor = .5;

  // Drawing related things
  gl::GLProgram* program = nullptr;
};


inline void registerRaySet(std::string name, const std::vector<std::vector<RayPoint>>& r, bool replaceIfPresent) {
  RaySet* s = new RaySet(name, r);
  bool success = registerStructure(s);
  if (!success) delete s;
}

inline RaySet* getRaySet(std::string name) {
  return dynamic_cast<RaySet*>(getStructure(RaySet::structureTypeName, name));
}

} // namespace polyscope

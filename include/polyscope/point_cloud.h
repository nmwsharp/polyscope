#pragma once

#include <vector>

#include "geometrycentral/vector3.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"

namespace polyscope {

class PointCloud : public Structure {
 public:
  // === Member functions ===

  // Construct a new point cloud structure
  PointCloud(std::string name,
             const std::vector<geometrycentral::Vector3>& points);
  ~PointCloud();

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;

  // Build the imgui display
  virtual void drawUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() override;

  bool enabled = true;

 private:
  // The points that make up this point cloud
  std::vector<geometrycentral::Vector3> points;

  // Visualization parameters
  std::array<float, 3> pointColor;
  float pointRadius = 0.005;

  // Drawing related things
  gl::GLProgram* program = nullptr;
};

}  // namespace polyscope
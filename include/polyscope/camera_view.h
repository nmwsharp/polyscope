#pragma once

#include <vector>

#include "geometrycentral/vector3.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"


namespace polyscope {

class CameraView : public Structure {
 public:
  // === Member functions ===

  // Construct a new surface mesh structure
  CameraView(std::string name, ...);
  ~CameraView();

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

  // === Quantity-related
//   void addQuantity(std::string name, VertexData<double>& value, DataType type = DataType::STANDARD);

  // === Helpers

  // === Member variables ===
  bool enabled = true;

  // The camera parameters
  glm::vec3 location;
  glm::mat3x3 rotation;
  glm::vec2 imageCenter;
  glm::vec2 focalLengths;
  

  // Drawing related things
  gl::GLProgram* cameraSkeleton = nullptr;
//   gl::GLProgram* program = nullptr;

 private:
  // Quantities

};


}

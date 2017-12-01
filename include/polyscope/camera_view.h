#pragma once

#include <vector>

#include "geometrycentral/vector3.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"
#include "polyscope/camera_parameters.h"


namespace polyscope {

class CameraView : public Structure {
 public:
  // === Member functions ===

  // Construct a new camera view structure
  CameraView(std::string name, CameraParameters p);
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
  void prepareCameraSkeleton();
  Vector3 location();

  // === Member variables ===
  bool enabled = true;

  // The camera parameters
  CameraParameters parameters;

  // Drawing related things
  gl::GLProgram* cameraSkeletonProgram = nullptr;
//   gl::GLProgram* program = nullptr;

 private:
  // Quantities

};


}

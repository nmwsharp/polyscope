#pragma once

#include <vector>

#include "geometrycentral/vector3.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"
#include "polyscope/camera_parameters.h"


namespace polyscope {

// Lightweight class to hold RGB image data
class Image {

public:
  Image(std::string name, unsigned char* data, size_t width, size_t height);
  ~Image();

  std::string name;
  size_t width, height;
  unsigned char* data;

};

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
  virtual void drawPickUI(size_t localPickID) override;
  virtual void preparePick() override;

  // Build the imgui display
  virtual void drawUI() override;
  virtual void drawSharedStructureUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() override;

  // === Quantity-related
  // Add an image with RGB components
  void setActiveImage(std::string name);
  void clearActiveImage();
  void addImage(std::string name, unsigned char* I, size_t width, size_t height);
  void removeImage(std::string name, bool errorIfNotPresent=false);

  // === Helpers
  void prepareCameraSkeleton();

  // Get the points that describe the "skeleton" in world space. Frame is CCW, starting with upper right.
  // dirFrame is {lookDir, upDir, rightDir}
  void getCameraPoints(Vector3& root, std::array<Vector3, 4>& framePoints, std::array<Vector3, 3>& dirFrame);
  Vector3 location();

  
  void drawWireframe();
  void drawImageView();

  static const std::string structureTypeName;

  // === Member variables ===
  bool enabled = true;

  // The camera parameters
  CameraParameters parameters;

 private:

  // Quantities
  std::map<std::string, Image*> images;
  
  // Drawing related things
  gl::GLProgram* cameraSkeletonProgram = nullptr;
  gl::GLProgram* pickProgram = nullptr;
  Image* activeImage = nullptr;
  gl::GLProgram* imageViewProgram = nullptr;
  static float globalImageTransparency;
  float cameraSkeletonScale; // the lengthscale cameras were first drawn with
  

};


}

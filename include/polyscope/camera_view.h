// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/camera_parameters.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include <vector>

namespace polyscope {

// Forward declare structure
class CameraView;

/*
// Forward declare quantity types (currently there are none)
template <> // Specialize the quantity type
struct QuantityTypeHelper<CameraView> {
  typedef CameraViewQuantity type;
};
*/

class CameraView : public QuantityStructure<CameraView> {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  CameraView(std::string name, const CameraParameters& params);

  // === Overrides

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // === Quantities

  // Scalars
  /*
  template <class T>
  CameraViewScalarQuantity* addScalarQuantity(std::string name, const T& values, DataType type = DataType::STANDARD);
  */


  // === Mutate

  // TODO oneday
  // void updateCameraParameters(const CameraParameters& newParams);
  // template <class V>
  // void updateCameraParameters(const V& newParameters);

  // get the params object
  CameraParameters getCameraParameters();

  // Misc data
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();

  // Update the current viewer to look through this camer
  void setViewToThisCamera(bool withFlight = false);

  // === Get/set visualization parameters

  // Set focal length of the camera. This only effects how it the camera widget is rendered
  // in the 3D view, it has nothing to do with the actual data stored or camera transform.
  CameraView* setDisplayFocalLength(double newVal, bool isRelative = true);
  double getDisplayFocalLength();

  // Set the thickness of the wireframe used to draw the camera (in relative units)
  CameraView* setDisplayThickness(double newVal);
  double getDisplayThickness();

  // Color of the widget
  CameraView* setWidgetColor(glm::vec3 val);
  glm::vec3 getWidgetColor();

  // Rendering helpers used by quantities
  void setCameraViewUniforms(render::ShaderProgram& p);
  std::vector<std::string> addCameraViewRules(std::vector<std::string> initRules, bool withCameraView = true);
  std::string getShaderNameForRenderMode();


private:
  // The actual camera data being visualized
  CameraParameters params;

  // === Visualization parameters
  PersistentValue<ScaledValue<float>> displayFocalLength;
  PersistentValue<float> displayThickness;
  PersistentValue<glm::vec3> widgetColor;

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::shared_ptr<render::ShaderProgram> nodeProgram, edgeProgram;
  std::shared_ptr<render::ShaderProgram> pickFrameProgram;

  // === Helpers
  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();
  void geometryChanged();
  void fillCameraWidgetGeometry(render::ShaderProgram* nodeProgram, render::ShaderProgram* edgeProgram,
                                render::ShaderProgram* pickFrameProgram);

  float displayFocalLengthUpper = -777;
  size_t pickStart = INVALID_IND;
  glm::vec3 pickColor;

  // === Quantity adder implementations
  /*
  CameraViewScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  */
};


// Shorthand to add a camera view to Polyscope
template <class T1, class T2, class T3>
CameraView* registerCameraView(std::string name, const T1& root, const T2& lookDir, const T3& upDir, double fovVertDeg,
                               double aspectRatio);

// Shorthand to get a point cloud from polyscope
inline CameraView* getCameraView(std::string name = "");
inline bool hasCameraView(std::string name = "");
inline void removeCameraView(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/camera_view.ipp"

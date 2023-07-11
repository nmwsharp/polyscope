// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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

  // To add images to the camera, use the add***ImageQuantity() functions
  // These are 'floating' quantaties that can be used with any structure, although they have extra features when used
  // with camera views.


  // === Mutate

  // Update the camera's location / parameters
  void updateCameraParameters(const CameraParameters& newParams);

  // get the params object
  CameraParameters getCameraParameters() const;

  // Misc data
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();

  // Update the current viewer to look through this camer
  void setViewToThisCamera(bool withFlight = false);

  // === Get/set visualization parameters

  // Set focal length of the camera. This only effects how it the camera widget is rendered
  // in the 3D view, it has nothing to do with the actual data stored or camera transform.
  CameraView* setWidgetFocalLength(float newVal, bool isRelative = true);
  float getWidgetFocalLength();

  // Set the thickness of the wireframe used to draw the camera (in relative units)
  CameraView* setWidgetThickness(float newVal);
  float getWidgetThickness();

  // Color of the widget
  CameraView* setWidgetColor(glm::vec3 val);
  glm::vec3 getWidgetColor();

  // Rendering helpers used by quantities
  void setCameraViewUniforms(render::ShaderProgram& p);
  std::vector<std::string> addCameraViewRules(std::vector<std::string> initRules, bool withCameraView = true);
  std::string getShaderNameForRenderMode();

  // Get info related to how the frame is drawn (billboard center vector, center-to-top vector, center-to-right vector)
  std::tuple<glm::vec3, glm::vec3, glm::vec3> getFrameBillboardGeometry();


private:
  // The actual camera data being visualized
  CameraParameters params;

  // === Visualization parameters
  PersistentValue<ScaledValue<float>> widgetFocalLength;
  PersistentValue<float> widgetThickness;
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

  float widgetFocalLengthUpper = -777;
  size_t pickStart = INVALID_IND;
  glm::vec3 pickColor;

  // track the length scale which was used to generate the camera geometry, in case it needs to be regenerated
  float preparedLengthScale = -1.;
  float pickPreparedLengthScale = -1.;

  // === Quantity adder implementations
};


// Shorthand to add a camera view to Polyscope
CameraView* registerCameraView(std::string name, CameraParameters params);

// Shorthand to get a point cloud from polyscope
inline CameraView* getCameraView(std::string name = "");
inline bool hasCameraView(std::string name = "");
inline void removeCameraView(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/camera_view.ipp"

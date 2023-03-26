// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "glm/glm.hpp"

#include "polyscope/persistent_value.h"
#include "polyscope/render/engine.h"
#include "polyscope/transformation_gizmo.h"


namespace polyscope {


// A 'structure' in Polyscope terms, is an object with which we can associate data in the UI, such as a point cloud,
// or a mesh. This in contrast to 'quantities', which we associate with the structures. For instance, a surface mesh
// (which is a structure), might have two scalar functions and a vector field associated with it (which are quantities).
//
// Polyscope expects that (1) the name member is unique of all structures with that type registered with the UI, and (2)
// the type member is a consistent name for the type (I strongly suggest setting it from a constant in the derived
// type). These two properties are not great polymorphic design, but they seem to be the lowest-effort way to allow a
// user to utilize and access custom structures with little code.


class Structure {

public:
  Structure(std::string name, std::string subtypeName);
  virtual ~Structure() = 0;

  // == Render the the structure on screen
  virtual void draw() = 0;
  virtual void drawDelayed() = 0;
  virtual void drawPick() = 0;

  // == Add rendering rules
  std::vector<std::string> addStructureRules(std::vector<std::string> initRules);

  // == Build the ImGUI ui elements
  virtual void buildUI();
  virtual void buildCustomUI() = 0;       // overridden by childen to add custom UI data
  virtual void buildCustomOptionsUI();    // overridden by childen to add to the options menu
  virtual void buildStructureOptionsUI(); // overridden by structure quantities to add to the options menu
  virtual void buildQuantitiesUI();       // build quantities, if they exist. Overridden by QuantityStructure.
  virtual void buildSharedStructureUI();  // Draw any UI elements shared between all instances of the structure
  virtual void buildPickUI(size_t localPickID) = 0; // Draw pick UI elements when index localPickID is selected

  // = Identifying data
  const std::string name; // should be unique amongst registered structures with this type
  std::string uniquePrefix();

  std::string getName() { return name; }; // used by pybind to access the name property

  // = Length and bounding box
  // (returned in world coordinates, after the object transform is applied)
  std::tuple<glm::vec3, glm::vec3> boundingBox(); // get axis-aligned bounding box
  float lengthScale();                            // get characteristic length
  virtual bool hasExtents();                      // bounding box and length scale are only meaningful if true

  // = Basic state
  virtual std::string typeName() = 0;

  // = Scene transform
  glm::mat4 getModelView();
  void centerBoundingBox();
  void rescaleToUnit();
  void resetTransform();
  void setTransform(glm::mat4x4 transform);
  void setPosition(glm::vec3 vec); // set the transform translation to be vec
  void translate(glm::vec3 vec);   // *adds* vec to the position
  glm::mat4x4 getTransform();
  glm::vec3 getPosition();

  void setStructureUniforms(render::ShaderProgram& p);
  bool wantsCullPosition();

  // Re-perform any setup work, including refreshing all quantities
  virtual void refresh();

  // Get rid of it (invalidates the object and all pointers, etc!)
  void remove();

  // Selection tools
  virtual Structure* setEnabled(bool newEnabled);
  bool isEnabled();
  void enableIsolate();                      // enable this structure, disable all of same type
  void setEnabledAllOfType(bool newEnabled); // enable/disable all structures of this type

  // Options
  Structure* setTransparency(float newVal); // also enables transparency if <1 and transparency is not enabled
  float getTransparency();

  Structure* setCullWholeElements(bool newVal);
  bool getCullWholeElements();

  Structure* setIgnoreSlicePlane(std::string name, bool newValue);
  bool getIgnoreSlicePlane(std::string name);

protected:
  // = State
  PersistentValue<bool> enabled;
  PersistentValue<glm::mat4> objectTransform; // rigid transform

  // 0 for transparent, 1 for opaque, only has effect if engine transparency is set
  PersistentValue<float> transparency;

  // Widget that wraps the transform
  TransformationGizmo transformGizmo;

  PersistentValue<bool> cullWholeElements;

  PersistentValue<std::vector<std::string>> ignoredSlicePlaneNames;

  // Manage the bounding box & length scale
  // (this is defined _before_ the object transform is applied. To get the scale/bounding box after transforms, use the
  // boundingBox() and lengthScale() member function)
  // The STRUCTURE is responsible for making sure updateObjectSpaceBounds() gets called any time the geometry changes
  std::tuple<glm::vec3, glm::vec3> objectSpaceBoundingBox;
  float objectSpaceLengthScale;
  virtual void updateObjectSpaceBounds() = 0;
};


// Register a structure with polyscope
// Structure name must be a globally unique identifier for the structure.
bool registerStructure(Structure* structure, bool replaceIfPresent = true);

// Can also manage quantities


// forward declarations
class Quantity;
template <typename S>
class QuantityS;

// Floating quantity things
class FloatingQuantity;
class ScalarImageQuantity;
class ColorImageQuantity;
class DepthRenderImageQuantity;
class ColorRenderImageQuantity;
class ScalarRenderImageQuantity;

// Helper used to define quantity types
template <typename T>
struct QuantityTypeHelper {
  typedef QuantityS<T> type; // default values
};

template <typename S> // template on the derived type
class QuantityStructure : public Structure {
public:
  // Nicer name for the quantity type of this structure
  typedef typename QuantityTypeHelper<S>::type QuantityType;

  // === Member functions ===

  // Base constructor which sets the name
  QuantityStructure(std::string name, std::string subtypeName);
  virtual ~QuantityStructure() = 0;

  virtual void buildQuantitiesUI() override;
  virtual void buildStructureOptionsUI() override;

  // Re-perform any setup work, including refreshing all quantities
  virtual void refresh() override;

  // = Manage quantities

  // Note: takes ownership of pointer after it is passed in
  void addQuantity(QuantityType* q, bool allowReplacement = true);
  void addQuantity(FloatingQuantity* q, bool allowReplacement = true);

  QuantityType*
  getQuantity(std::string name); // NOTE: will _not_ return floating quantities, must use other version below
  FloatingQuantity* getFloatingQuantity(std::string name);
  void removeQuantity(std::string name, bool errorIfAbsent = false);
  void removeAllQuantities();

  void setDominantQuantity(QuantityS<S>* q);
  void clearDominantQuantity();

  void setAllQuantitiesEnabled(bool newEnabled);

  // = Quantities
  std::map<std::string, std::unique_ptr<QuantityType>> quantities;
  QuantityS<S>* dominantQuantity = nullptr; // If non-null, a special quantity of which only one can be drawn for
                                            // the structure. Handles common case of a surface color, e.g. color of
                                            // a mesh or point cloud. The dominant quantity must always be enabled.

  // floating quantities are tracked separately from normal quantities, though names should still be unique etc
  std::map<std::string, std::unique_ptr<FloatingQuantity>> floatingQuantities;

  // === Floating Quantities
  template <class T>
  ScalarImageQuantity* addScalarImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values,
                                              ImageOrigin imageOrigin = ImageOrigin::UpperLeft,
                                              DataType type = DataType::STANDARD);

  template <class T>
  ColorImageQuantity* addColorImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgb,
                                            ImageOrigin imageOrigin = ImageOrigin::UpperLeft);

  template <class T>
  ColorImageQuantity* addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgba,
                                                 ImageOrigin imageOrigin = ImageOrigin::UpperLeft);

  template <class T1, class T2>
  DepthRenderImageQuantity* addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                        const T2& normalData,
                                                        ImageOrigin imageOrigin = ImageOrigin::UpperLeft);

  template <class T1, class T2, class T3>
  ColorRenderImageQuantity* addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                        const T2& normalData, const T3& colorData,
                                                        ImageOrigin imageOrigin = ImageOrigin::UpperLeft);


  template <class T1, class T2, class T3>
  ScalarRenderImageQuantity*
  addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData, const T2& normalData,
                               const T3& scalarData, ImageOrigin imageOrigin = ImageOrigin::UpperLeft,
                               DataType type = DataType::STANDARD);


  // === Floating Quantity impls
  ScalarImageQuantity* addScalarImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                  const std::vector<double>& values, ImageOrigin imageOrigin,
                                                  DataType type);

  ColorImageQuantity* addColorImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                const std::vector<glm::vec4>& values, ImageOrigin imageOrigin);

  DepthRenderImageQuantity* addDepthRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                            const std::vector<float>& depthData,
                                                            const std::vector<glm::vec3>& normalData,
                                                            ImageOrigin imageOrigin);

  ColorRenderImageQuantity* addColorRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                            const std::vector<float>& depthData,
                                                            const std::vector<glm::vec3>& normalData,
                                                            const std::vector<glm::vec3>& colorData,
                                                            ImageOrigin imageOrigin);

  ScalarRenderImageQuantity* addScalarRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                              const std::vector<float>& depthData,
                                                              const std::vector<glm::vec3>& normalData,
                                                              const std::vector<double>& scalarData,
                                                              ImageOrigin imageOrigin, DataType type);

protected:
  // helper
  bool checkForQuantityWithNameAndDeleteOrError(std::string name, bool allowReplacement);
};


} // namespace polyscope

#include "polyscope/structure.ipp"

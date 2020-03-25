// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/quantity.h"
#include "polyscope/render/engine.h"

#include "glm/glm.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <string>


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
  virtual void drawPick() = 0;

  // == Build the ImGUI ui elements
  void buildUI();
  virtual void buildCustomUI() = 0;      // overridden by childen to add custom UI data
  virtual void buildCustomOptionsUI();   // overridden by childen to add to the options menu
  virtual void buildQuantitiesUI();      // build quantities, if they exist. Overridden by QuantityStructure.
  virtual void buildSharedStructureUI(); // Draw any UI elements shared between all instances of the structure
  virtual void buildPickUI(size_t localPickID) = 0; // Draw pick UI elements when index localPickID is selected

  // = Identifying data
  const std::string name; // should be unique amongst registered structures with this type
  std::string uniquePrefix();

  // = Length and bounding box (returned in object coordinates)
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() = 0; // get axis-aligned bounding box
  virtual double lengthScale() = 0;                           // get characteristic length

  // = Basic state
  virtual Structure* setEnabled(bool newEnabled);
  bool isEnabled();
  virtual std::string typeName() = 0;

  // = Scene transform
  glm::mat4 objectTransform = glm::mat4(1.0);
  glm::mat4 getModelView();
  void centerBoundingBox();
  void rescaleToUnit();
  void resetTransform();
  void setTransformUniforms(render::ShaderProgram& p);

  // Get rid of it (invalidates the object and all pointers, etc!)
  void remove();

protected:
  // = State
  PersistentValue<bool> enabled;
};


// Can also manage quantities

// Helper used to define quantity types
template <typename T>
struct QuantityTypeHelper {
  typedef Quantity<T> type; // default values
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

  // = Manage quantities

  // Note: takes ownership of pointer after it is passed in
  void addQuantity(QuantityType* q, bool allowReplacement = true);

  QuantityType* getQuantity(std::string name);
  void removeQuantity(std::string name);
  void removeAllQuantities();

  void setDominantQuantity(Quantity<S>* q);
  void clearDominantQuantity();

  // = Quantities
  std::map<std::string, std::unique_ptr<QuantityType>> quantities;
  Quantity<S>* dominantQuantity = nullptr; // If non-null, a special quantity of which only one can be drawn for
                                           // the structure. Handles common case of a surface color, e.g. color of
                                           // a mesh or point cloud. The dominant quantity must always be enabled.
};


} // namespace polyscope

#include "polyscope/structure.ipp"

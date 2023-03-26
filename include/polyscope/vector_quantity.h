#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/scaled_value.h"
#include "polyscope/types.h"


namespace polyscope {

// These classes encapsulate logic which is common to all vector quantities

// ================================================
// === Base Vector Quantity
// ================================================

// Common base class for the 3D and 2D/tangent sub-cases below

template <typename QuantityT>
class VectorQuantityBase {
public:
  VectorQuantityBase(QuantityT& parent, VectorType vectorType);

  // Build the ImGUI UIs for vectors
  void buildVectorUI();

  // === Members
  QuantityT& quantity;

  // === Option accessors

  //  The vectors will be scaled such that the longest vector is this long
  QuantityT* setVectorLengthScale(double newLength, bool isRelative = true);
  double getVectorLengthScale();

  // The radius of the vectors
  QuantityT* setVectorRadius(double val, bool isRelative = true);
  double getVectorRadius();

  // The color of the vectors
  QuantityT* setVectorColor(glm::vec3 color);
  glm::vec3 getVectorColor();

  // Material
  QuantityT* setMaterial(std::string name);
  std::string getMaterial();


protected:
  const VectorType vectorType;

  // === Visualization options
  PersistentValue<ScaledValue<float>> vectorLengthMult;
  PersistentValue<ScaledValue<float>> vectorRadius;
  PersistentValue<glm::vec3> vectorColor;
  PersistentValue<std::string> material;

  std::shared_ptr<render::ShaderProgram> vectorProgram;

  // Manages _actually_ drawing the vectors, generating gui.
  // TODO
  // std::unique_ptr<VectorArtist> vectorArtist;
  // void prepareVectorArtist();
};

// ================================================
// === (3D) Vector Quantity
// ================================================

// 3D vectors sitting in space

template <typename QuantityT>
class VectorQuantity : public VectorQuantityBase<QuantityT> {
public:
  VectorQuantity(QuantityT& parent, const std::vector<glm::vec3>& vectors,
                 render::ManagedBuffer<glm::vec3>& vectorRoots, VectorType vectorType);

  void drawVectors();
  void refreshVectors();

  template <class V>
  void updateData(const V& newVectors);
  template <class V>
  void updateData2D(const V& newVectors);

  // === Members

  // Wrapper around the actual buffer of vector data stored in the class.
  // Interaction with the data (updating it on CPU or GPU side, accessing it, etc) happens through this wrapper.
  render::ManagedBuffer<glm::vec3> vectors;


  // A buffer of root locations at which to draw the vectors. Not that this is _not_ owned by this class, it is just a
  // reference.
  render::ManagedBuffer<glm::vec3>& vectorRoots;


  // === ~DANGER~ experimental/unsupported functions


protected:
  // helpers
  void createProgram();
  void updateMaxLength();

  std::vector<glm::vec3> vectorsData;
  float maxLength = -777;
};


// ================================================
// === Tangent Vector Quantity
// ================================================

// Tangent vectors defined in some tangent space

template <typename QuantityT>
class TangentVectorQuantity : public VectorQuantityBase<QuantityT> {
public:
  TangentVectorQuantity(QuantityT& parent, const std::vector<glm::vec2>& tangentVectors,
                        render::ManagedBuffer<glm::vec3>& vectorRoots,
                        render::ManagedBuffer<std::array<glm::vec3, 2>>& tangentBasis, VectorType vectorType);

  void drawVectors();
  void refreshVectors();

  template <class V>
  void updateData(const V& newVectors);

  // === Members

  // Wrapper around the actual buffer of vector data stored in the class.
  // Interaction with the data (updating it on CPU or GPU side, accessing it, etc) happens through this wrapper.
  render::ManagedBuffer<glm::vec2> tangentVectors;

  // A buffer of root locations at which to draw the vectors. Not that this is _not_ owned by this class, it is just a
  // reference.
  render::ManagedBuffer<glm::vec3>& vectorRoots;

  // A buffer of (orthonormal) tangent frames at which to draw the vectors. Again, just a reference.
  render::ManagedBuffer<std::array<glm::vec3, 2>>& tangentBasis;

  // === ~DANGER~ experimental/unsupported functions

protected:
  // helpers
  void createProgram();
  void updateMaxLength();

  std::vector<glm::vec2> tangentVectorsData;
  float maxLength = -777;
};

} // namespace polyscope


#include "polyscope/vector_quantity.ipp"

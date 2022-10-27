#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/scaled_value.h"

namespace polyscope {

// Encapsulates logic which is common to all scalar quantities

template <typename QuantityT>
class VectorQuantity {
public:
  VectorQuantity(QuantityT& parent, const std::vector<glm::vec3>& vectors, VectorType vectorType);

  // Build the ImGUI UIs for scalars
  void buildVectorUI();

  void drawVectors();
  void refreshVectors();

  template <class V>
  void updateData(const V& newVectors);
  template <class V>
  void updateData2D(const V& newVectors);

  // === Members
  QuantityT& quantity;

  bool vectorsStoredInMemory();
  size_t nVectorSize();
  glm::vec3 getVector(size_t ind);

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


  // === ~DANGER~ experimental/unsupported functions

  std::shared_ptr<render::AttributeBuffer> getVectorRenderBuffer();
  void renderBufferDataExternallyUpdated();


protected:
  // helpers
  void ensureRenderBuffersFilled(bool forceRefill = false);
  void dataUpdated();
  void createProgram();
  void updateMaxLength();

  std::vector<glm::vec3> vectors;
  const VectorType vectorType;
  double maxLength = -1;

  // === Visualization options
  PersistentValue<ScaledValue<float>> vectorLengthMult;
  PersistentValue<ScaledValue<float>> vectorRadius;
  PersistentValue<glm::vec3> vectorColor;
  PersistentValue<std::string> material;


  std::shared_ptr<render::AttributeBuffer> vectorRenderBuffer;
  std::shared_ptr<render::AttributeBuffer> baseRenderBuffer;
  std::shared_ptr<render::ShaderProgram> vectorProgram;
};

} // namespace polyscope


#include "polyscope/vector_quantity.ipp"

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
  VectorQuantity(QuantityT& parent, const std::vector<double>& values, DataType dataType);

  // Build the ImGUI UIs for scalars
  void buildScalarUI();
  void buildScalarOptionsUI(); // called inside of an options menu

  // Add rules to rendering programs for scalars
  std::vector<std::string> addScalarRules(std::vector<std::string> rules);

  // Set uniforms in rendering programs for scalars
  void setScalarUniforms(render::ShaderProgram& p);

  // === Members
  QuantityT& quantity;
  std::vector<glm::vec3> vectors;
  const DataType dataType;

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
  double maxLength = -1;

  // === Visualization options
  PersistentValue<ScaledValue<float>> vectorLengthMult;
  PersistentValue<ScaledValue<float>> vectorRadius;
  PersistentValue<glm::vec3> vectorColor;
  PersistentValue<std::string> material;


  std::shared_ptr<render::ShaderProgram> program;

  // helpers
  void createProgram();
  void updateMaxLength();
};

} // namespace polyscope


#include "polyscope/vector_quantity.ipp"

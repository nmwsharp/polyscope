// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/structure.h"


namespace polyscope {


// A utility class for drawing vectors.
// Note: Does not actually take ownership of memory buffers for vectors; just keeps a reference to the buffer, which
// must stay valid.

class VectorArtist {

public:
  VectorArtist(Structure& parentStructure_, std::string uniqueName_, const std::vector<glm::vec3>& bases_,
               const std::vector<glm::vec3>& vectors_, const VectorType& vectorType_);

  void draw();
  void buildParametersUI();


  // === Option accessors

  //  The vectors will be scaled such that the longest vector is this long
  void setVectorLengthScale(double newLength, bool isRelative = true);
  double getVectorLengthScale();

  // The radius of the vectors
  void setVectorRadius(double val, bool isRelative = true);
  double getVectorRadius();

  // The color of the vectors
  void setVectorColor(glm::vec3 color);
  glm::vec3 getVectorColor();

  // Material
  void setMaterial(std::string name);
  std::string getMaterial();

private:
  // Data
  Structure& parentStructure;
  const std::string uniqueName;
  const std::string uniquePrefix;
  const VectorType vectorType;
  const std::vector<glm::vec3>& bases;
  const std::vector<glm::vec3>& vectors;
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

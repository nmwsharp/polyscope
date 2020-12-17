// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/point_cloud.h"
#include "polyscope/vector_artist.h"

namespace polyscope {

// Represents a general vector field associated with a point cloud
class PointCloudVectorQuantity : public PointCloudQuantity {
public:
  PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors, PointCloud& pointCloud_,
                           VectorType vectorType_ = VectorType::STANDARD);
  
  std::vector<glm::vec3> vectors;

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t ind) override;
  virtual std::string niceName() override;
  virtual void refresh() override;

  // === Members
  // Note: these vectors are not the raw vectors passed in by the user, but have been rescaled such that the longest has
  // length 1 (unless type is VectorType::Ambient)
  const VectorType vectorType;

  // Note: all of the actual work is done by the vector artist

  // === Option accessors

  //  The vectors will be scaled such that the longest vector is this long
  PointCloudVectorQuantity* setVectorLengthScale(double newLength, bool isRelative = true);
  double getVectorLengthScale();

  // The radius of the vectors
  PointCloudVectorQuantity* setVectorRadius(double val, bool isRelative = true);
  double getVectorRadius();

  // The color of the vectors
  PointCloudVectorQuantity* setVectorColor(glm::vec3 color);
  glm::vec3 getVectorColor();

  // Material
  PointCloudVectorQuantity* setMaterial(std::string name);
  std::string getMaterial();

private:

  // Manages _actually_ drawing the vectors, generating gui.
  std::unique_ptr<VectorArtist> vectorArtist;
};

} // namespace polyscope

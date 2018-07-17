#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/point_cloud.h"

namespace polyscope {

// Represents a general vector field associated with a point cloud
class PointCloudVectorQuantity : public PointCloudQuantity {
public:
  PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors, PointCloud* pointCloud_,
                           VectorType vectorType_ = VectorType::STANDARD);

  virtual ~PointCloudVectorQuantity() override;

  virtual void draw() override;
  virtual void drawUI() override;
  virtual void buildInfoGUI(size_t ind) override;

  // === Members
  const VectorType vectorType;
  std::vector<glm::vec3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  Color3f vectorColor;


  // The map that takes values to [0,1] for drawing
  AffineRemapper<glm::vec3> mapper;

  void writeToFile(std::string filename = "");

  // GL things
  void prepare();
  gl::GLProgram* program = nullptr;
};

template <class T>
void PointCloud::addVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {

  validateSize(vectors, nPoints(), "point cloud vector quantity " + name);

  PointCloudQuantity* q =
      new PointCloudVectorQuantity(name, standardizeVectorArray<glm::vec3, T, 3>(vectors), this, vectorType);
  addQuantity(q);
}


} // namespace polyscope

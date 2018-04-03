#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/point_cloud.h"

namespace polyscope {

// Represents a general vector field associated with a point cloud
class PointCloudVectorQuantity : public PointCloudQuantity {
public:
  PointCloudVectorQuantity(std::string name, std::vector<Vector3> vectors, PointCloud* pointCloud_,
                           VectorType vectorType_ = VectorType::STANDARD);

  virtual ~PointCloudVectorQuantity() override;

  virtual void draw() override;
  virtual void drawUI() override;
  virtual void buildInfoGUI(size_t ind) override;

  // === Members
  const VectorType vectorType;
  std::vector<Vector3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  std::array<float, 3> vectorColor;


  // The map that takes values to [0,1] for drawing
  AffineRemapper<Vector3> mapper;
  
  void writeToFile(std::string filename = "");

  // GL things
  void prepare();
  gl::GLProgram* program = nullptr;
};


} // namespace polyscope

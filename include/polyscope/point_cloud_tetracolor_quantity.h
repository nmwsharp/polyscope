#pragma once

#include "polyscope/tetracolor_quantity.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include <vector>

namespace polyscope {

class PointCloudTetracolorQuantity : public PointCloudQuantity, public TetracolorQuantity<PointCloudTetracolorQuantity> {
public:
  PointCloudTetracolorQuantity(std::string name, const std::vector<glm::vec4>& values_, PointCloud& pointCloud_);

  virtual void draw() override;
  
  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;

  virtual std::string niceName() override;

  void createPointProgram();

protected:
  std::shared_ptr<render::ShaderProgram> pointProgram;

}; // class PointCloudTetracolorQuantity  

} // namespace polyscope

#pragma once

#include "polyscope/tetracolor_quantity.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include <vector>

namespace polyscope {

class PointCloudTetracolorQuantity : public PointCloudQuantity, public TetracolorQuantity<PointCloudTetracolorQuantity> {
public:
  PointCloudTetracolorQuantity(std::string name, const std::vector<glm::vec4>& values_, PointCloud& pointCloud_);

  // Draw this tetracolor quantity.
  virtual void draw() override;
  
  // Get readable name for UI.
  virtual std::string niceName() override;

  virtual void buildPickUI(size_t ind) override;

  virtual void refresh() override;

protected:
  // Shader name is determined by whether we are rendering spheres or quads. 
  std::string getShaderNameForRenderMode();

  // Create the ShaderProgram that will draw this tetracolor quantity.
  void createPointProgram();

  // The ShaderProgram that will draw this tetracolor quantity.
  std::shared_ptr<render::ShaderProgram> pointProgram;

}; // class PointCloudTetracolorQuantity  

} // namespace polyscope

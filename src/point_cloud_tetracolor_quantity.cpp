
#include "polyscope/point_cloud_tetracolor_quantity.h"
#include "polyscope/polyscope.h"

namespace polyscope {

PointCloudTetracolorQuantity::PointCloudTetracolorQuantity(std::string name, const std::vector<glm::vec4>& values_,
                                                           PointCloud& pointCloud_)
  : PointCloudQuantity(name, pointCloud_, true), TetracolorQuantity(*this, values_) {}

} // namespace polyscope

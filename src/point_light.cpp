
#include "polyscope/point_light.h"

namespace polyscope {

PointLight::PointLight(std::string name_, glm::vec3 lightPosition_, glm::vec3 lightColor_)
  : name(name_), lightPosition(lightPosition_), lightColor(lightColor_) {}

} // namespace polyscope

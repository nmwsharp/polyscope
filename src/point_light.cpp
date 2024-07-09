
#include "polyscope/point_light.h"

namespace polyscope {

PointLight::PointLight(std::string name_, glm::vec3 position_, glm::vec3 color_)
  : name(name_) {
  data = new PointLightData(position_, color_, true);
}

PointLight::~PointLight() {}

} // namespace polyscope

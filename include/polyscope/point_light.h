
#pragma once

#include "polyscope/light.h"

namespace polyscope {

class PointLight : public Light {
public:
  PointLight(std::string name, glm::vec3 position, glm::vec3 color);
  ~PointLight();

  std::string getTypeName() override;

  static const std::string lightTypeName;

}; // class PointLight

PointLight* registerPointLight(std::string name, glm::vec3 position, glm::vec3 color);

} // namespace polyscope

#include "polyscope/point_light.ipp"

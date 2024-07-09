
#pragma once

#include "glm/vec3.hpp"
#include <string>

namespace polyscope {
// Struct for data that will be passed to shader programs
struct PointLightData {
  glm::vec3 position;
  glm::vec3 color;
  bool enabled;

  PointLightData(glm::vec3 position_, glm::vec3 color_, bool val)
    : position(position_), color(color_), enabled(val) {}
}; // struct PointLightData

class PointLight {
public:
  PointLight(std::string name_, glm::vec3 position_, glm::vec3 color_);
  ~PointLight();

  void updateLightPosition(glm::vec3 newPosition);
  void updateLightColor(glm::vec3 newColor);
  void setEnabled(bool newVal);
  bool isEnabled();

private:
  std::string name;
  PointLightData* data;
}; // class PointLight

PointLight* registerPointLight(std::string name, glm::vec3 position, glm::vec3 color = glm::vec3{1.0, 1.0, 1.0});
inline void removePointLight(std::string name);

} // namespace polyscope

#include "polyscope/point_light.ipp"

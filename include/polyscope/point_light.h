
#pragma once

#include "polyscope/polyscope.h"

namespace polyscope {

// TODO: PointLight should eventually inherit from Light
class PointLight {
public:
  PointLight(std::string name_, glm::vec3 lightPosition_, glm::vec3 lightColor_);

private:
  std::string name;
  // TODO: should these be PersistentValues?
  glm::vec3 lightPosition;
  glm::vec3 lightColor;

}; // class PointLight

// Add a point light to polyscope
PointLight* registerPointLight(std::string name, glm::vec3 lightPosition, glm::vec3 lightColor = glm::vec3{0.0, 0.0, 0.0});

// Get a point light from polyscope
inline PointLight* getPointLight(std::string name = "");
inline bool hasPointLight(std::string name = "");
inline void removePointLight(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/point_light.ipp"

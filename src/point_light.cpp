
#include "polyscope/point_light.h"

namespace polyscope {

const std::string PointLight::lightTypeName = "Point Light";

PointLight::PointLight(std::string name, glm::vec3 position, glm::vec3 color)
  : Light(name, position, color) {}

PointLight::~PointLight() {}

PointLight* PointLight::setLightPosition(glm::vec3 newPos) {
  position= newPos;
  render::engine->lightManager->setLightPosition(name, newPos);
  return this;
}

PointLight* PointLight::setLightColor(glm::vec3 newCol) {
  color = newCol;
  render::engine->lightManager->setLightColor(name, newCol);
  return this;
}

PointLight* PointLight::setEnabled(bool newVal) {
  enabled = newVal;
  render::engine->lightManager->setEnabled(name, newVal);
  return this;
}

std::string PointLight::getTypeName() { return lightTypeName; }

PointLight* registerPointLight(std::string name, glm::vec3 position, glm::vec3 color) {
  checkInitialized();

  PointLight* s = new PointLight(name, position, color);
  
  bool success = registerLight(s);
  if (!success) {
    // TODO: safeDelete(s)
  }

  return s;
}

} // namespace polyscope

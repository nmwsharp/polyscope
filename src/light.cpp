
#include "polyscope/light.h"

namespace polyscope {

Light::Light(std::string name, glm::vec3 position, glm::vec3 color)
  : name(name), position(position), color(color) {}

Light::~Light() {}

bool Light::isEnabled() { return enabled; }

std::string Light::getLightName() { return name; }

glm::vec3 Light::getLightPosition() { return position; }

glm::vec3 Light::getLightColor() { return color; }

} // namespace polyscope

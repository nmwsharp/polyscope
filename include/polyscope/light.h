
#pragma once

#include "polyscope/polyscope.h"

#include <string>
#include <iostream>

#include "glm/glm.hpp"

namespace polyscope {

class Light {
public:
  Light(std::string name, glm::vec3 position, glm::vec3 color);
  virtual ~Light();

  virtual Light* setLightPosition(glm::vec3 newPos) = 0;
  virtual Light* setLightColor(glm::vec3 newCol) = 0;
  virtual Light* setEnabled(bool newVal) = 0;
  virtual std::string getTypeName() = 0;

  bool isEnabled();
  std::string getLightName();
  glm::vec3 getLightPosition();
  glm::vec3 getLightColor();

protected:
  std::string name;
  glm::vec3 position;
  glm::vec3 color;
  bool enabled;

}; // class Light

// Register a Light with Polyscope.
bool registerLight(Light* light, bool replaceIfPresent = true);

} // namespace polyscope

#include "polyscope/light.ipp"

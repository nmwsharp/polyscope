
#pragma once

#include "polyscope/light.h"

namespace polyscope {

class PointLight : public Light {
public:
  PointLight(std::string name, glm::vec3 position, glm::vec3 color);
  ~PointLight();

  PointLight* setLightPosition(glm::vec3 newPos) override;
  PointLight* setLightColor(glm::vec3 newCol) override;
  PointLight* setEnabled(bool newVal) override;

  std::string getTypeName() override;

  static const std::string lightTypeName;

}; // class PointLight

PointLight* registerPointLight(std::string name, glm::vec3 position, glm::vec3 color);

inline PointLight* getPointLight(std::string name = "");
inline bool hasPointLight(std::string name = "");
inline void removePointLight(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/point_light.ipp"

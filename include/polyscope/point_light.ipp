
#pragma once

namespace polyscope {

inline PointLight* getPointLight(std::string name) {
  return dynamic_cast<PointLight*>(getLight(PointLight::lightTypeName, name));
} 

inline bool hasPointLight(std::string name) {
  return hasLight(PointLight::lightTypeName, name);
}

inline void removePointLight(std::string name, bool errorIfAbsent) {
  removeLight(PointLight::lightTypeName, name, errorIfAbsent);
}

} // namespace polyscope

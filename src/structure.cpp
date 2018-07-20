#include "polyscope/structure.h"

#include "polyscope/polyscope.h"

namespace polyscope {

Structure::Structure(std::string name_, std::string type_) : name(name_), type(type_) {}

Structure::~Structure(){};


void Structure::resetTransform() {
  objectTransform = glm::mat4(1.0);
  updateStructureExtents();
}

void Structure::centerBoundingBox() {
  std::tuple<glm::vec3, glm::vec3> bbox = boundingBox();
  glm::vec3 center = (std::get<1>(bbox) + std::get<0>(bbox)) / 2.0f;
  glm::mat4x4 newTrans = glm::translate(glm::mat4x4(1.0), -glm::vec3(center.x, center.y, center.z));
  objectTransform = objectTransform * newTrans;
  updateStructureExtents();
}

glm::mat4 Structure::getModelView() { return view::getCameraViewMatrix() * objectTransform; }

} // namespace polyscope

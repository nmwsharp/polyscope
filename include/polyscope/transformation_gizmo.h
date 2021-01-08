#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"
#include "polyscope/widget.h"

namespace polyscope {

// A visual widget with handles for translations/rotations

class TransformationGizmo : public Widget {

public:
  // == Constructors

  TransformationGizmo(std::string name, glm::mat4& T, PersistentValue<glm::mat4>* Tpers = nullptr);


  // == Key members

  // a unique name
  const std::string name;

  // the main transform encoded by the gizmo
  PersistentValue<bool> enabled;

  // the main transform encoded by the gizmo
  // note that this is a reference set on construction; the gizmo wraps a transform defined somewhere else
  glm::mat4& T;
  PersistentValue<glm::mat4>* Tpers; // optional, a persistent value defined elsewhere that goes with T

  // == Member functions

  void prepare();
  void draw() override;
  bool interact() override;

protected:
  enum class TransformHandle { None, Rotation, Translation, Scale };


  // parameters
  const float gizmoSizeRel = 0.08;
  const float diskWidthObj = 0.1; // in object coordinates, before transformation
  const float vecLength = 1.5;
  const float sphereRad = 0.32;

  // state
  int selectedDim = -1; // must be {0,1,2} if selectedType == Rotation/Translation
  TransformHandle selectedType = TransformHandle::None;
  bool currentlyDragging = false;
  glm::vec3 dragPrevVec{1., 0.,
                        0.}; // the normal vector from the previous frame of the drag OR previous translation center

  std::array<glm::vec3, 3> niceRGB = {glm::vec3{211 / 255., 45 / 255., 62 / 255.},
                                      glm::vec3{65 / 255., 121 / 255., 225 / 255.},
                                      glm::vec3{95 / 255., 175 / 255., 35 / 255.}};

  void markUpdated();

  // Render stuff
  std::shared_ptr<render::ShaderProgram> ringProgram;
  std::shared_ptr<render::ShaderProgram> arrowProgram;
  std::shared_ptr<render::ShaderProgram> sphereProgram;

  // Geometry helpers used to test hits

  // returns <tRay, distNearest, nearestPoint>
  std::tuple<float, float, glm::vec3> circleTest(glm::vec3 raySource, glm::vec3 rayDir, glm::vec3 center,
                                                 glm::vec3 normal, float radius);
  std::tuple<float, float, glm::vec3> lineTest(glm::vec3 raySource, glm::vec3 rayDir, glm::vec3 center,
                                               glm::vec3 tangent, float length);
  std::tuple<float, float, glm::vec3> sphereTest(glm::vec3 raySource, glm::vec3 rayDir, glm::vec3 center, float radius,
                                                 bool allowHitSurface = true);


  std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec2>,
             std::vector<glm::vec3>>
  triplePlaneCoords();

  std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>>
  tripleArrowCoords();

  // std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>> unitCubeCoords();
};

} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <polyscope/types.h>
#include <polyscope/weak_handle.h>


#include <glm/glm.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>


namespace polyscope {


// forward declarations
class Structure;
class Group;
class SlicePlane;
class Widget;
class FloatingQuantityStructure;
namespace view {
extern const double defaultNearClipRatio;
extern const double defaultFarClipRatio;
extern const double defaultFov;
} // namespace view

// A context object wrapping all global state used by Polyscope.
//
// In theory the user could explicitly manage multiple contexts explicitly. However for now that is not supported, there
// is always exactly one global context.
//
// Historically, these globals were simply `static` members scattered through a few different files. However, this
// was a persistent source of bugs at shutdown time, because the order in which destructors are called during shutdown
// is platform-dependent. Bugs often arose because one global member often depends on another; if destructed in an
// unexpected order, they would reference one-another and cause platform-dependent errors. The global context solves
// this because destruction always happens in a predictable order.

struct Context {

  // ======================================================
  // === General globals from polyscope.h
  // ======================================================

  bool initialized = false;
  std::string backend = "";
  std::map<std::string, std::map<std::string, std::unique_ptr<Structure>>> structures;
  std::map<std::string, std::unique_ptr<Group>> groups;
  float lengthScale = 1.;
  std::tuple<glm::vec3, glm::vec3> boundingBox =
      std::tuple<glm::vec3, glm::vec3>{glm::vec3{-1., -1., -1.}, glm::vec3{1., 1., 1.}};
  std::vector<std::unique_ptr<SlicePlane>> slicePlanes;
  std::vector<WeakHandle<Widget>> widgets;
  bool doDefaultMouseInteraction = true;
  std::function<void()> userCallback = nullptr;


  // ======================================================
  // === Render engine globals from engine.h
  // ======================================================


  // ======================================================
  // === View globals from view.h
  // ======================================================

  int bufferWidth = -1;
  int bufferHeight = -1;
  int windowWidth = -1;  // on init(), get overwritten with defaultWindowWidth if -1
  int windowHeight = -1; // ^^^ same
  int initWindowPosX = 20;
  int initWindowPosY = 20;
  bool windowResizable = true;
  NavigateStyle navigateStyle = NavigateStyle::Turntable;
  UpDir upDir = UpDir::YUp;
  FrontDir frontDir = FrontDir::ZFront;
  double moveScale = 1.0;
  double nearClipRatio = view::defaultNearClipRatio;
  double farClipRatio = view::defaultFarClipRatio;
  std::array<float, 4> bgColor{{1.0, 1.0, 1.0, 0.0}};
  glm::mat4x4 viewMat;
  double fov = view::defaultFov;
  ProjectionMode projectionMode = ProjectionMode::Perspective;
  bool midflight = false;
  float flightStartTime = -1;
  float flightEndTime = -1;
  glm::dualquat flightTargetViewR, flightInitialViewR;
  glm::vec3 flightTargetViewT, flightInitialViewT;
  float flightTargetFov, flightInitialFov;


  // ======================================================
  // === Internal globals from internal.h
  // ======================================================

  bool pointCloudEfficiencyWarningReported = false;
  FloatingQuantityStructure* globalFloatingQuantityStructure = nullptr;
};


}; // namespace polyscope

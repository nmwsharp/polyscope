#include "polyscope/point_cloud.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/ray_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

// Initialize static
const std::string RaySet::structureTypeName = "Ray Set";

RaySet::RaySet(std::string name, const std::vector<std::vector<RayPoint>>& r_)
    : Structure(name, RaySet::structureTypeName), rayPaths(r_) {

  baseColor = getNextStructureColor();
  rayColor = baseColor;
  colorManager = SubColorManager(baseColor);

  prepare();
}

RaySet::~RaySet() {
  if (program != nullptr) {
    delete program;
  }
}

void RaySet::draw() {
  if (!enabled) {
    return;
  }

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  // Ray parameterization related
  float timeMax = 1.0 / viewIntervalFactor * state::lengthScale;
  program->setUniform("u_timeMax", timeMax);
  float t = glm::mod(ImGui::GetTime() * speedFactor * 2, timeMax);
  program->setUniform("u_time", t);
  float streakWidth = 0.3 * streakLengthFactor * state::lengthScale;
  program->setUniform("u_streakWidth", streakWidth);
  
  

  program->setUniform("u_color", rayColor);

  program->draw();
}

void RaySet::drawPick() {}

void RaySet::prepare() {

  // Randomize the path order to avoid quirks
  // std::shuffle(rayPaths.begin(), rayPaths.end());

  // Create the GL program
  program = new gl::GLProgram(&RAY_VERT_SHADER, &RAY_GEOM_SHADER, &RAY_FRAG_SHADER,
                              gl::DrawMode::Lines);

  // Attributes to fill
  std::vector<Vector3> points;
  std::vector<double> times;
  std::vector<double> offsets;

  for (const std::vector<RayPoint>& path : rayPaths) {
    if (path.size() < 2) {
      error("Ray paths must have at least two entries to make sense");
      continue;
    }

    // Generate a random offset on [0,1]
    double offset = unitRand();
    double currT = 0;

    // Validate
    if (path[0].isInfiniteDirection) {
      error("Infinite ray direction should only occur at end of path");
      continue;
    }

    for (size_t iP = 0; iP < path.size() - 1; iP++) {
      const RayPoint& pCurr = path[iP];
      const RayPoint& pNext = path[iP + 1];

      Vector3 pStart = pCurr.v;
      Vector3 pEnd = pNext.v;

      // Special case for infinite rays
      if (pNext.isInfiniteDirection) {
        if (iP == path.size() - 2) {
          // Special case for ending in an infinite ray
          // Shooot off in to distance
          // TODO do somethign better (this won't update as lengthscale is changed, which is bad)
          pEnd = pStart + 20 * state::lengthScale * pNext.v;

        } else {
          // Validate
          error("Infinite ray direction should only occur at end of path");
          break;
        }
      }

      // General case: another point in the path
      double len = norm(pStart - pEnd);

      // First point
      points.push_back(pStart);
      times.push_back(currT);
      offsets.push_back(offset);

      // Second point
      points.push_back(pEnd);
      times.push_back(currT + len);
      offsets.push_back(offset);

      currT += len;
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", points);
  program->setAttribute("a_tp", times);
  program->setAttribute("a_offset", offsets);

  // Set a reasonable default value for the default view factor
  float maxReasonableViewRays = 10000;
  viewIntervalFactor = clamp(maxReasonableViewRays / (.5 * points.size()), 0.0, 1.0); 
}

void RaySet::preparePick() {}

void RaySet::drawSharedStructureUI() {}

void RaySet::drawPickUI(size_t localPickID) {}

void RaySet::drawUI() {
  if (ImGui::TreeNode(name.c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Ray color", (float*)&rayColor,
                      ImGuiColorEditFlags_NoInputs);
  
    ImGui::SliderFloat("Prune fraction", &viewIntervalFactor, 0.0, 1.0, "%.5f", 3.);
    ImGui::SliderFloat("Speed", &speedFactor, 0.0, 1., "%.3f", 3);
    ImGui::SliderFloat("Streak length", &streakLengthFactor, 0.0, .1, "%.5f", 3.);

    ImGui::TreePop();
  }
}

double RaySet::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding
  // box
  auto bound = boundingBox();
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (const auto& path : rayPaths) {
    for (const RayPoint& p : path) {
      if (!p.isInfiniteDirection) {
        lengthScale =
            std::max(lengthScale, geometrycentral::norm2(p.v - center));
      }
    }
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
RaySet::boundingBox() {
  Vector3 min = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (const auto& path : rayPaths) {
    for (const RayPoint& p : path) {
      if (!p.isInfiniteDirection) {
        min = geometrycentral::componentwiseMin(min, p.v);
        max = geometrycentral::componentwiseMax(max, p.v);
      }
    }
  }

  return std::make_tuple(min, max);
}

}  // namespace polyscope

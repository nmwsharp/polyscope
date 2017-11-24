#include "polyscope/point_cloud.h"

using namespace geometrycentral;

namespace polyscope {

PointCloud::PointCloud(std::string name, const std::vector<Vector3>& points_)
    : Structure(name), points(points_) {}

void PointCloud::draw() {}

void PointCloud::drawPick() {}

void PointCloud::prepare() {}

void PointCloud::teardown() {}

void PointCloud::drawUI() {}

double PointCloud::lengthScale() {
  return 1;  // TODO
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
PointCloud::boundingBox() {
  return {Vector3{0, 0, 0}, Vector3{1, 1, 1}};  // TODO
}

}  // namespace polyscope
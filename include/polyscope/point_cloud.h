#pragma once

#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

// Note extra quantity includes at bottom

namespace polyscope {

// Forward declare point cloud
class PointCloud;

class PointCloudQuantity : public Quantity<PointCloud> {
public:
  // Base constructor which sets the name
  PointCloudQuantity(std::string name, PointCloud& pointCloud, bool dominates = false);
  virtual ~PointCloudQuantity() = 0;

  // Build GUI info about a point
  virtual void buildInfoGUI(size_t pointInd);
};

// Specific subclass indicating that a quantity will create a program to draw on the points themselves
class PointCloudQuantityThatDrawsPoints : public PointCloudQuantity {

public:
  PointCloudQuantityThatDrawsPoints(std::string name, PointCloud& pointCloud);

protected:
  std::unique_ptr<gl::GLProgram> pointProgram;
};


class PointCloud : public QuantityStructure<PointCloud> {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  template <class T>
  PointCloud(std::string name, const T& points);
  ~PointCloud();

  // === Overloads

  // Render the the structure on screen
  virtual void draw() override;

  // Build the imgui display
  virtual void drawCustomUI() override;
  virtual void drawCustomOptionsUI() override;
  virtual void drawPickUI(size_t localPickID) override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;

  virtual std::string typeName() override;

  // === Quantities

  PointCloudQuantity* getQuantity(std::string name, bool errorIfAbsent = true);

  // Scalars
  template <class T>
  void addScalarQuantity(std::string name, const T& values, DataType type = DataType::STANDARD);

  // Colors
  template <class T>
  void addColorQuantity(std::string name, const T& values);

  // Subsets
  // void addSubsetQuantity(std::string name, const std::vector<char>& subsetIndicators);
  // void addSubsetQuantity(std::string name, const std::vector<size_t>& subsetIndices);

  // Vectors
  template <class T>
  void addVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD);

  // The points that make up this point cloud
  std::vector<glm::vec3> points;
  size_t nPoints() const { return points.size(); }

  // Misc data
  SubColorManager colorManager;
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();
  void writePointsToFile(std::string filename = "");
  void setPointCloudUniforms(gl::GLProgram& p);

private:
  // Visualization parameters
  Color3f initialBaseColor;
  Color3f pointColor;
  float pointRadius = 0.005;

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::unique_ptr<gl::GLProgram> program;
  std::unique_ptr<gl::GLProgram> pickProgram;

  // === Helpers
  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();
};


// Implementation of templated constructor
template <class T>
PointCloud::PointCloud(std::string name, const T& points_)
    : QuantityStructure<PointCloud>(name), points(standardizeVectorArray<glm::vec3, T, 3>(points_)) {

  initialBaseColor = getNextStructureColor();
  pointColor = initialBaseColor;
  colorManager = SubColorManager(initialBaseColor);
}


// Shorthand to add a point cloud to polyscope
template <class T>
void registerPointCloud(std::string name, const T& points, bool replaceIfPresent = true) {
  PointCloud* s = new PointCloud(name, points);
  bool success = registerStructure(s);
  if (!success) delete s;
}


// Shorthand to get a point cloud from polyscope
inline PointCloud* getPointCloud(std::string name = "") {
  return dynamic_cast<PointCloud*>(getStructure(PointCloud::structureTypeName, name));
}


} // namespace polyscope


// Quantity includes
#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"


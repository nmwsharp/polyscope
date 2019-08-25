// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/point_cloud_quantity.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include <vector>

namespace polyscope {

// Forward declare point cloud
class PointCloud;

// Forward declare quantity types
class PointCloudColorQuantity;
class PointCloudScalarQuantity;
class PointCloudVectorQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<PointCloud> {
  typedef PointCloudQuantity type;
};

class PointCloud : public QuantityStructure<PointCloud> {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  PointCloud(std::string name, std::vector<glm::vec3> points);

  // === Overloads

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Render the the structure on screen
  virtual void draw() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;

  virtual std::string typeName() override;

  // === Quantities

  // Scalars
  template <class T>
  PointCloudScalarQuantity* addScalarQuantity(std::string name, const T& values, DataType type = DataType::STANDARD);

  // Colors
  template <class T>
  PointCloudColorQuantity* addColorQuantity(std::string name, const T& values);

  // Vectors
  template <class T>
  PointCloudVectorQuantity* addVectorQuantity(std::string name, const T& vectors,
                                              VectorType vectorType = VectorType::STANDARD);
  template <class T>
  PointCloudVectorQuantity* addVectorQuantity2D(std::string name, const T& vectors,
                                                VectorType vectorType = VectorType::STANDARD);
  
  // === Mutate
  template <class V>
  void updatePointPositions(const V& newPositions);
  template <class V>
  void updatePointPositions2D(const V& newPositions);

  // The points that make up this point cloud
  std::vector<glm::vec3> points;
  size_t nPoints() const { return points.size(); }

  // Misc data
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();
  void writePointsToFile(std::string filename = "");
  void setPointCloudUniforms(gl::GLProgram& p);

  // Visualization parameters
  glm::vec3 initialBaseColor;
  glm::vec3 pointColor;
  float pointRadius = 0.005;

private:

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::unique_ptr<gl::GLProgram> program;
  std::unique_ptr<gl::GLProgram> pickProgram;

  // === Helpers
  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();


  // === Quantity adder implementations
  PointCloudScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  PointCloudColorQuantity* addColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  PointCloudVectorQuantity* addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors,
                                                  VectorType vectorType);
};


// Shorthand to add a point cloud to polyscope
template <class T>
PointCloud* registerPointCloud(std::string name, const T& points, bool replaceIfPresent = true);
template <class T>
PointCloud* registerPointCloud2D(std::string name, const T& points, bool replaceIfPresent = true);

// Shorthand to get a point cloud from polyscope
inline PointCloud* getPointCloud(std::string name = "");


} // namespace polyscope

#include "polyscope/point_cloud.ipp"

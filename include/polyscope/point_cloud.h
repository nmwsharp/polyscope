#pragma once

#include <vector>

#include "geometrycentral/vector3.h"
#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"

namespace polyscope {

// Forward declare point cloud
class PointCloud;

class PointCloudQuantity {
public:
  // Base constructor which sets the name
  PointCloudQuantity(std::string name, PointCloud* pointCloud);
  virtual ~PointCloudQuantity() = 0;

  // Draw the quantity on the surface Note: for many quantities (like scalars)
  // this does nothing, because drawing happens in the mesh draw(). However
  // others (ie vectors) need to be drawn.
  virtual void draw();

  // Draw the ImGUI ui elements
  virtual void drawUI() = 0;

  // Build GUI info about a point
  virtual void buildInfoGUI(size_t pointInd);

  // === Member variables ===
  const std::string name;
  PointCloud* const parent;

  bool enabled = false; // should be set by enable() and disable()
};

// Specific subclass indicating that a quantity can create a program to draw on the points themselves
class PointCloudQuantityThatDrawsPoints : public PointCloudQuantity {
public:
  PointCloudQuantityThatDrawsPoints(std::string name, PointCloud* pointCloud);
  // Create a program to be used for drawing the points
  // CALLER is responsible for deallocating
  virtual gl::GLProgram* createProgram() = 0;

  // Do any per-frame work on the program handed out by createProgram
  virtual void setProgramValues(gl::GLProgram* program);
  virtual bool wantsBillboardUniforms();
};


class PointCloud : public Structure {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  PointCloud(std::string name, const std::vector<geometrycentral::Vector3>& points);
  ~PointCloud();

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;
  virtual void preparePick() override;

  // Build the imgui display
  virtual void drawUI() override;
  virtual void drawPickUI(size_t localPickID) override;
  virtual void drawSharedStructureUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> boundingBox() override;


  // === Quantities

  // general form
  void addQuantity(PointCloudQuantity* quantity);
  void addQuantity(PointCloudQuantityThatDrawsPoints* quantity);
  PointCloudQuantity* getQuantity(std::string name, bool errorIfAbsent = true);

  // Scalars
  void addScalarQuantity(std::string name, const std::vector<double>& value, DataType type = DataType::STANDARD);

  // Colors
  void addColorQuantity(std::string name, const std::vector<Vector3>& value);

  // Subsets
  // void addSubsetQuantity(std::string name, const std::vector<char>& subsetIndicators);
  // void addSubsetQuantity(std::string name, const std::vector<size_t>& subsetIndices);

  // Vectors
  void addVectorQuantity(std::string name, const std::vector<Vector3>& value,
                         VectorType vectorType = VectorType::STANDARD);


  // Removal, etc
  void removeQuantity(std::string name);
  void setActiveQuantity(PointCloudQuantityThatDrawsPoints* q);
  void clearActiveQuantity();
  void removeAllQuantities();

  // The points that make up this point cloud
  std::vector<geometrycentral::Vector3> points;
  
  // Misc data
  bool enabled = true;
  SubColorManager colorManager;
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();
  void writePointsToFile(std::string filename = "");

  void setUseBillboardSpheres(bool newValue);
  bool requestsBillboardSpheres() const;

private:
  // Quantities
  std::map<std::string, PointCloudQuantity*> quantities;

  // Visualization parameters
  Color3f initialBaseColor;
  Color3f pointColor;
  float pointRadius = 0.005;
  bool useBillboardSpheres = false;

  // Drawing related things
  gl::GLProgram* program = nullptr;
  gl::GLProgram* pickProgram = nullptr;

  PointCloudQuantityThatDrawsPoints* activePointQuantity = nullptr; // a quantity that is respondible for drawing on the
                                                                    // points themselves and overwrites `program` with
                                                                    // its own shaders

  // Helpers
  void setPointCloudUniforms(gl::GLProgram* p, bool withLight, bool withBillboard);
};

} // namespace polyscope

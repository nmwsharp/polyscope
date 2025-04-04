// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/persistent_value.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud_quantity.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_parameterization_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

#include <vector>

namespace polyscope {

// Forward declare point cloud
class PointCloud;

// Forward declare quantity types
class PointCloudColorQuantity;
class PointCloudScalarQuantity;
class PointCloudParameterizationQuantity;
class PointCloudVectorQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<PointCloud> {
  typedef PointCloudQuantity type;
};

struct PointCloudPickResult {
  int64_t index;
};

class PointCloud : public QuantityStructure<PointCloud> {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  PointCloud(std::string name, std::vector<glm::vec3> points);

  // === Overrides

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(const PickResult& result) override;

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // === Geometry members
  render::ManagedBuffer<glm::vec3> points;

  // === Quantities

  // Scalars
  template <class T>
  PointCloudScalarQuantity* addScalarQuantity(std::string name, const T& values, DataType type = DataType::STANDARD);

  // Parameterization
  template <class T>
  PointCloudParameterizationQuantity* addParameterizationQuantity(std::string name, const T& values,
                                                                  ParamCoordsType type = ParamCoordsType::UNIT);
  template <class T>
  PointCloudParameterizationQuantity* addLocalParameterizationQuantity(std::string name, const T& values,
                                                                       ParamCoordsType type = ParamCoordsType::WORLD);

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

  // === Set point size from a scalar quantity
  // effect is multiplicative with pointRadius
  // negative values are always clamped to 0
  // if autoScale==true, values are rescaled such that the largest has size 1
  void setPointRadiusQuantity(PointCloudScalarQuantity* quantity, bool autoScale = true);
  void setPointRadiusQuantity(std::string name, bool autoScale = true);
  void clearPointRadiusQuantity();

  // === Set transparency alpha from a scalar quantity
  // effect is multiplicative with other transparency values
  // values are clamped to [0,1]
  void setTransparencyQuantity(PointCloudScalarQuantity* quantity);
  void setTransparencyQuantity(std::string name);
  void clearTransparencyQuantity();

  // The points that make up this point cloud
  // Normally, the values are stored here. But if the render buffer
  // is being manually updated, they will live only in the render buffer
  // and this will be empty.
  size_t nPoints();
  glm::vec3 getPointPosition(size_t iPt);

  // get data related to picking/selection
  PointCloudPickResult interpretPickResult(const PickResult& result);

  // Misc data
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();

  // === Get/set visualization parameters

  // set the base color of the points
  PointCloud* setPointRenderMode(PointRenderMode newVal);
  PointRenderMode getPointRenderMode();

  // set the base color of the points
  PointCloud* setPointColor(glm::vec3 newVal);
  glm::vec3 getPointColor();

  // set the radius of the points
  PointCloud* setPointRadius(double newVal, bool isRelative = true);
  double getPointRadius();

  // Material
  PointCloud* setMaterial(std::string name);
  std::string getMaterial();

  // Rendering helpers used by quantities
  void setPointCloudUniforms(render::ShaderProgram& p);
  void setPointProgramGeometryAttributes(render::ShaderProgram& p);
  std::vector<std::string> addPointCloudRules(std::vector<std::string> initRules, bool withPointCloud = true);
  std::string getShaderNameForRenderMode();

  // === ~DANGER~ experimental/unsupported functions


private:
  // Storage for the managed buffers above. You should generally interact with this directly through them.
  std::vector<glm::vec3> pointsData;

  // === Visualization parameters
  PersistentValue<std::string> pointRenderMode;
  PersistentValue<glm::vec3> pointColor;
  PersistentValue<ScaledValue<float>> pointRadius;
  PersistentValue<std::string> material;

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // === Helpers
  // Do setup work related to drawing, including allocating openGL data
  void ensureRenderProgramPrepared();
  void ensurePickProgramPrepared();

  // === Quantity adder implementations
  PointCloudScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType type);
  PointCloudParameterizationQuantity*
  addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& param, ParamCoordsType type);
  PointCloudParameterizationQuantity*
  addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& param, ParamCoordsType type);
  PointCloudColorQuantity* addColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  PointCloudVectorQuantity* addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors,
                                                  VectorType vectorType);

  // Manage varying point size
  // which (scalar) quantity to set point size from
  // TODO make these PersistentValue<>?
  std::string pointRadiusQuantityName = ""; // empty string means none
  bool pointRadiusQuantityAutoscale = true;
  PointCloudScalarQuantity& resolvePointRadiusQuantity(); // helper

  // Manage per-element transparency
  // which (scalar) quantity to set point size from
  // TODO make these PersistentValue<>?
  std::string transparencyQuantityName = "";               // empty string means none
  PointCloudScalarQuantity& resolveTransparencyQuantity(); // helper
};

// Shorthand to add a point cloud to polyscope
template <class T>
PointCloud* registerPointCloud(std::string name, const T& points);
template <class T>
PointCloud* registerPointCloud2D(std::string name, const T& points);

// Shorthand to get a point cloud from polyscope
inline PointCloud* getPointCloud(std::string name = "");
inline bool hasPointCloud(std::string name = "");
inline void removePointCloud(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/point_cloud.ipp"

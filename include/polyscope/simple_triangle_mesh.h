// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_management.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/scaled_value.h"
#include "polyscope/structure.h"

#include <vector>

namespace polyscope {

// Forward declare simple triangle mesh
class SimpleTriangleMesh;

// Forward declare quantity types

// template <> // Specialize the quantity type
// struct QuantityTypeHelper<SimpleTriangleMesh> {
//   typedef SimpleTriangleMeshQuantity type;
// };

class SimpleTriangleMesh : public QuantityStructure<SimpleTriangleMesh> {
public:
  // === Member functions ===

  // Construct a new simple triangle mesh structure
  SimpleTriangleMesh(std::string name, std::vector<glm::vec3> vertices, std::vector<glm::uvec3> faces);

  // === Overrides

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // === Geometry members
  render::ManagedBuffer<glm::vec3> vertices;
  render::ManagedBuffer<glm::uvec3> faces;

  // === Quantities

  // === Mutate

  template <class V>
  void updateVertices(const V& newPositions);

  template <class V, class F>
  void update(const V& newVertices, const F& newFaces);

  // Misc data
  static const std::string structureTypeName;

  // === Get/set visualization parameters

  // set the base color of the surface
  SimpleTriangleMesh* setSurfaceColor(glm::vec3 newVal);
  glm::vec3 getSurfaceColor();

  // Material
  SimpleTriangleMesh* setMaterial(std::string name);
  std::string getMaterial();

  // Backface color
  SimpleTriangleMesh* setBackFaceColor(glm::vec3 val);
  glm::vec3 getBackFaceColor();

  // Backface policy
  SimpleTriangleMesh* setBackFacePolicy(BackFacePolicy newPolicy);
  BackFacePolicy getBackFacePolicy();

  // Rendering helpers used by quantities
  void setSimpleTriangleMeshUniforms(render::ShaderProgram& p, bool withSurfaceShade = true);
  void setSimpleTriangleMeshProgramGeometryAttributes(render::ShaderProgram& p);
  std::vector<std::string> addSimpleTriangleMeshRules(std::vector<std::string> initRules, bool withSurfaceShade = true);

  // === ~DANGER~ experimental/unsupported functions


private:
  // Storage for the managed buffers above. You should generally interact with this directly through them.
  std::vector<glm::vec3> verticesData;
  std::vector<glm::uvec3> facesData;

  // === Visualization parameters
  PersistentValue<glm::vec3> surfaceColor;
  PersistentValue<std::string> material;
  PersistentValue<BackFacePolicy> backFacePolicy;
  PersistentValue<glm::vec3> backFaceColor;

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // === Helpers
  // Do setup work related to drawing, including allocating openGL data
  void ensureRenderProgramPrepared();
  void ensurePickProgramPrepared();
  void setPickUniforms(render::ShaderProgram& p);

  // === Quantity adder implementations

  // == Picking related things
  size_t pickStart;
  glm::vec3 pickColor;
};


// Shorthand to add a simple triangle mesh to polyscope
template <class V, class F>
SimpleTriangleMesh* registerSimpleTriangleMesh(std::string name, const V& vertices, const F& faces);

// Shorthand to get a simple triangle mesh from polyscope
inline SimpleTriangleMesh* getSimpleTriangleMesh(std::string name = "");
inline bool hasSimpleTriangleMesh(std::string name = "");
inline void removeSimpleTriangleMesh(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/simple_triangle_mesh.ipp"

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string SimpleTriangleMesh::structureTypeName = "Simple Triangle Mesh";

// Constructor
SimpleTriangleMesh::SimpleTriangleMesh(std::string name, std::vector<glm::vec3> vertices_,
                                       std::vector<glm::uvec3> faces_)
    : // clang-format off
      QuantityStructure<SimpleTriangleMesh>(name, structureTypeName),
      vertices(this, uniquePrefix() + "vertices", verticesData), 
      faces(this, uniquePrefix() + "faces", facesData), 
      verticesData(std::move(vertices_)),
      facesData(std::move(faces_)),
      surfaceColor(uniquePrefix() + "surfaceColor", getNextUniqueColor()),
      material(uniquePrefix() + "material", "clay"),
      backFacePolicy(uniquePrefix() + "backFacePolicy", BackFacePolicy::Different),
      backFaceColor(uniquePrefix() + "backFaceColor", glm::vec3(1.f - surfaceColor.get().r, 1.f - surfaceColor.get().g, 1.f - surfaceColor.get().b))
// clang-format on
{
  cullWholeElements.setPassive(false);
  updateObjectSpaceBounds();
}

void SimpleTriangleMesh::buildCustomUI() {

  // Print stats
  long long int nVertsL = static_cast<long long int>(vertices.size());
  long long int nFacesL = static_cast<long long int>(faces.size());
  ImGui::Text("#verts: %lld  #faces: %lld", nVertsL, nFacesL);

  { // Colors
    if (ImGui::ColorEdit3("Color", &surfaceColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setSurfaceColor(surfaceColor.get());
  }

  { // Backface color (only visible if policy is selected)
    if (backFacePolicy.get() == BackFacePolicy::Custom) {
      if (ImGui::ColorEdit3("Backface Color", &backFaceColor.get()[0], ImGuiColorEditFlags_NoInputs))
        setBackFaceColor(backFaceColor.get());
    }
  }
}


void SimpleTriangleMesh::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }

  // backfaces
  if (ImGui::BeginMenu("Back Face Policy")) {
    if (ImGui::MenuItem("identical shading", NULL, backFacePolicy.get() == BackFacePolicy::Identical))
      setBackFacePolicy(BackFacePolicy::Identical);
    if (ImGui::MenuItem("different shading", NULL, backFacePolicy.get() == BackFacePolicy::Different))
      setBackFacePolicy(BackFacePolicy::Different);
    if (ImGui::MenuItem("custom shading", NULL, backFacePolicy.get() == BackFacePolicy::Custom))
      setBackFacePolicy(BackFacePolicy::Custom);
    if (ImGui::MenuItem("cull", NULL, backFacePolicy.get() == BackFacePolicy::Cull))
      setBackFacePolicy(BackFacePolicy::Cull);
    ImGui::EndMenu();
  }
}

void SimpleTriangleMesh::buildPickUI(size_t localPickID) {
  // Do nothing for now, we just pick a single constant for the whole structure
}

void SimpleTriangleMesh::draw() {
  if (!isEnabled()) {
    return;
  }

  if (getCullWholeElements()) setCullWholeElements(false); // whole elements not supported
  render::engine->setBackfaceCull(backFacePolicy.get() == BackFacePolicy::Cull);

  // If there is no dominant quantity, then this class is responsible for drawing points
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    ensureRenderProgramPrepared();

    // Set program uniforms
    setStructureUniforms(*program);
    setSimpleTriangleMeshUniforms(*program);
    render::engine->setMaterialUniforms(*program, material.get());
    program->setUniform("u_baseColor", surfaceColor.get());

    // Draw the actual point cloud
    program->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void SimpleTriangleMesh::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}

void SimpleTriangleMesh::drawPick() {
  if (!isEnabled()) {
    return;
  }

  // Ensure we have prepared buffers
  ensurePickProgramPrepared();

  if (getCullWholeElements()) setCullWholeElements(false); // whole elements not supported
  render::engine->setBackfaceCull(backFacePolicy.get() == BackFacePolicy::Cull);

  // Set uniforms
  setStructureUniforms(*pickProgram);
  setSimpleTriangleMeshUniforms(*pickProgram, false);
  setPickUniforms(*pickProgram);

  pickProgram->draw();
}

void SimpleTriangleMesh::setSimpleTriangleMeshUniforms(render::ShaderProgram& p, bool withSurfaceShade) {

  // for the tri-flat shading
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());

  if (withSurfaceShade) {

    if (backFacePolicy.get() == BackFacePolicy::Custom) {
      p.setUniform("u_backfaceColor", getBackFaceColor());
    }
  }
}

void SimpleTriangleMesh::ensureRenderProgramPrepared() {
  // If already prepared, do nothing
  if (program) return;

  // clang-format off
  program = render::engine->requestShader("SIMPLE_MESH", 
    render::engine->addMaterialRules(getMaterial(),
      addSimpleTriangleMeshRules(
        {
          "SHADE_BASECOLOR", "COMPUTE_SHADE_NORMAL_FROM_POSITION", "PROJ_AND_INV_PROJ_MAT"
        }
      )
    )
  );
  // clang-format on

  setSimpleTriangleMeshProgramGeometryAttributes(*program);

  render::engine->setMaterial(*program, getMaterial());
}

void SimpleTriangleMesh::ensurePickProgramPrepared() {

  // If already prepared, do nothing
  if (pickProgram) return;

  // clang-format off
  pickProgram = render::engine->requestShader("SIMPLE_MESH", 
    addSimpleTriangleMeshRules(
      {
        "SHADECOLOR_FROM_UNIFORM", "COMPUTE_SHADE_NORMAL_FROM_POSITION", "PROJ_AND_INV_PROJ_MAT"
      }
    , false), render::ShaderReplacementDefaults::Pick
  );
  // clang-format on

  setSimpleTriangleMeshProgramGeometryAttributes(*pickProgram);

  // Request pick indices
  pickStart = pick::requestPickBufferRange(this, 1);
  pickColor = pick::indToVec(pickStart);
}

void SimpleTriangleMesh::setPickUniforms(render::ShaderProgram& p) { p.setUniform("u_color", pickColor); }

std::vector<std::string> SimpleTriangleMesh::addSimpleTriangleMeshRules(std::vector<std::string> initRules,
                                                                        bool withSurfaceShade) {

  initRules = addStructureRules(initRules);

  if (withSurfaceShade) {
    // rules that only get used when we're shading the surface of the mesh

    if (backFacePolicy.get() == BackFacePolicy::Different) {
      initRules.push_back("MESH_BACKFACE_DARKEN");
    }
    if (backFacePolicy.get() == BackFacePolicy::Custom) {
      initRules.push_back("MESH_BACKFACE_DIFFERENT");
    }
  }

  if (backFacePolicy.get() == BackFacePolicy::Identical) {
    initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
  }

  if (backFacePolicy.get() == BackFacePolicy::Different) {
    initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
  }

  if (backFacePolicy.get() == BackFacePolicy::Custom) {
    initRules.push_back("MESH_BACKFACE_NORMAL_FLIP");
  }

  return initRules;
}

void SimpleTriangleMesh::setSimpleTriangleMeshProgramGeometryAttributes(render::ShaderProgram& p) {
  p.setAttribute("a_vertexPositions", vertices.getRenderAttributeBuffer());
  p.setIndex(faces.getRenderAttributeBuffer());
}


void SimpleTriangleMesh::refresh() {
  program.reset();
  pickProgram.reset();
  requestRedraw();
  QuantityStructure<SimpleTriangleMesh>::refresh(); // call base class version, which refreshes quantities
}

void SimpleTriangleMesh::updateObjectSpaceBounds() {

  vertices.ensureHostBufferPopulated();

  // bounding box
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  for (const glm::vec3& p : vertices.data) {
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }
  objectSpaceBoundingBox = std::make_tuple(min, max);

  // length scale, as twice the radius from the center of the bounding box
  glm::vec3 center = 0.5f * (min + max);
  float lengthScale = 0.0;
  for (const glm::vec3& p : vertices.data) {
    lengthScale = std::max(lengthScale, glm::length2(p - center));
  }
  objectSpaceLengthScale = 2 * std::sqrt(lengthScale);
}

std::string SimpleTriangleMesh::typeName() { return structureTypeName; }

// === Option getters and setters


SimpleTriangleMesh* SimpleTriangleMesh::setSurfaceColor(glm::vec3 val) {
  surfaceColor = val;
  requestRedraw();
  return this;
}
glm::vec3 SimpleTriangleMesh::getSurfaceColor() { return surfaceColor.get(); }

SimpleTriangleMesh* SimpleTriangleMesh::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string SimpleTriangleMesh::getMaterial() { return material.get(); }

SimpleTriangleMesh* SimpleTriangleMesh::setBackFacePolicy(BackFacePolicy newPolicy) {
  backFacePolicy = newPolicy;
  refresh();
  requestRedraw();
  return this;
}
BackFacePolicy SimpleTriangleMesh::getBackFacePolicy() { return backFacePolicy.get(); }

SimpleTriangleMesh* SimpleTriangleMesh::setBackFaceColor(glm::vec3 val) {
  backFaceColor = val;
  requestRedraw();
  return this;
}

glm::vec3 SimpleTriangleMesh::getBackFaceColor() { return backFaceColor.get(); }


} // namespace polyscope

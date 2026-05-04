// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/simple_triangle_mesh_color_quantity.h"
#include "polyscope/simple_triangle_mesh_scalar_quantity.h"
#include "polyscope/view.h"

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
      Structure(name, structureTypeName),
      vertices(this, uniquePrefix() + "vertices", verticesData), 
      faces(this, uniquePrefix() + "faces", facesData), 
      verticesData(std::move(vertices_)),
      facesData(std::move(faces_)),
      surfaceColor(uniquePrefix() + "surfaceColor", getNextUniqueColor()),
      material(uniquePrefix() + "material", "clay"),
      backFacePolicy(uniquePrefix() + "backFacePolicy", BackFacePolicy::Different),
      backFaceColor(uniquePrefix() + "backFaceColor", glm::vec3(1.f - surfaceColor.get().r, 1.f - surfaceColor.get().g, 1.f - surfaceColor.get().b)),
      selectionMode(uniquePrefix() + "selectionMode", MeshSelectionMode::Auto)
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

  // Selection mode
  if (ImGui::BeginMenu("Selection Mode")) {
    if (ImGui::MenuItem("auto", NULL, selectionMode.get() == MeshSelectionMode::Auto))
      setSelectionMode(MeshSelectionMode::Auto);
    if (ImGui::MenuItem("vertices only", NULL, selectionMode.get() == MeshSelectionMode::VerticesOnly))
      setSelectionMode(MeshSelectionMode::VerticesOnly);
    if (ImGui::MenuItem("faces only", NULL, selectionMode.get() == MeshSelectionMode::FacesOnly))
      setSelectionMode(MeshSelectionMode::FacesOnly);
    ImGui::EndMenu();
  }
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

  for (auto& x : quantities) {
    x.second->drawPick();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawPick();
  }
}

void SimpleTriangleMesh::drawPickDelayed() {
  if (!isEnabled()) {
    return;
  }

  for (auto& x : quantities) {
    x.second->drawPickDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawPickDelayed();
  }
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
          "SHADE_BASECOLOR"
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
    addSimpleTriangleMeshRules({"SIMPLE_MESH_PROPAGATE_FACE_PICK"}, false),
    render::ShaderReplacementDefaults::Pick);
  // clang-format on

  setSimpleTriangleMeshProgramGeometryAttributes(*pickProgram);

  // Allocate one pick index per face
  pickStart = pick::requestPickBufferRange(this, nFaces());
}

void SimpleTriangleMesh::setPickUniforms(render::ShaderProgram& p) {
  // Encode pickStart as two uint uniforms for the shader's 64-bit index arithmetic.
  // NOTE: must stay in sync with SIMPLE_MESH_PROPAGATE_FACE_PICK in surface_mesh_shaders.cpp
  // and with pick::indToVec() / pick::bitsForPickPacking in pick.ipp.
  p.setUniform("u_pickStartLow", static_cast<uint32_t>(pickStart & 0xFFFFFFFFull));
  p.setUniform("u_pickStartHigh", static_cast<uint32_t>(pickStart >> 32));
}

std::vector<std::string> SimpleTriangleMesh::addSimpleTriangleMeshRules(std::vector<std::string> initRules,
                                                                        bool withSurfaceShade) {

  initRules = addStructureRules(initRules);

  initRules.push_back("COMPUTE_SHADE_NORMAL_FROM_POSITION");
  initRules.push_back("PROJ_AND_INV_PROJ_MAT");

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
  Structure::refresh(); // call base class version, which refreshes quantities
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

void SimpleTriangleMesh::reserve(size_t nVerts, size_t nFaces) {
  verticesData.reserve(nVerts);
  facesData.reserve(nFaces);
}

SimpleTriangleMeshPickResult SimpleTriangleMesh::interpretPickResult(const PickResult& rawResult) {

  if (rawResult.structure != this) {
    exception("called interpretPickResult(), but the pick result is not from this structure");
  }

  SimpleTriangleMeshPickResult result;

  size_t localInd = rawResult.localIndex;
  if (localInd >= nFaces()) {
    // Shouldn't happen, but guard against stale pick data
    return result;
  }

  // Recover the 3 vertex positions of the clicked face
  glm::uvec3 triInds = faces.getValue(localInd);
  glm::vec3 pA = vertices.getValue(triInds[0]);
  glm::vec3 pB = vertices.getValue(triInds[1]);
  glm::vec3 pC = vertices.getValue(triInds[2]);

  // Compute barycentric coordinates of the pick via sub-triangle areas.
  // Each lambda_i is the area of the sub-triangle formed by the other two vertices and the click point P,
  // divided by the total.  Clamp each area to be nonneg and finite, then normalise. If the total is zero, fall back to
  // (1/3, 1/3, 1/3).
  glm::vec3 P = rawResult.position;

  auto subArea = [](glm::vec3 u, glm::vec3 v, glm::vec3 w) -> float {
    float a = glm::length(glm::cross(v - u, w - u));
    return (std::isfinite(a) && a >= 0.f) ? a : 0.f;
  };

  float aA = subArea(P, pB, pC);
  float aB = subArea(pA, P, pC);
  float aC = subArea(pA, pB, P);
  float total = aA + aB + aC;

  float lambdaA, lambdaB, lambdaC;
  if (total == 0.f) {
    lambdaA = lambdaB = lambdaC = 1.f / 3.f;
  } else {
    lambdaA = aA / total;
    lambdaB = aB / total;
    lambdaC = aC / total;
  }

  // Find the dominant vertex
  size_t nearestLocalIdx = 0;
  float maxLambda = lambdaA;
  if (lambdaB > maxLambda) {
    maxLambda = lambdaB;
    nearestLocalIdx = 1;
  }
  if (lambdaC > maxLambda) {
    maxLambda = lambdaC;
    nearestLocalIdx = 2;
  }
  size_t nearestVertexIdx = triInds[nearestLocalIdx];

  // Threshold: lambda must exceed this to count as a vertex click.
  // Matches SurfaceMesh: Auto uses 1 - 0.2 = 0.8, VerticesOnly always picks vertex, FacesOnly never does.
  float vertexPickThreshold;
  switch (selectionMode.get()) {
  case MeshSelectionMode::Auto:
    vertexPickThreshold = 0.8f;
    break;
  case MeshSelectionMode::VerticesOnly:
    vertexPickThreshold = 0.0f;
    break;
  case MeshSelectionMode::FacesOnly:
    vertexPickThreshold = 2.0f;
    break;
  default:
    vertexPickThreshold = 0.8f;
    break;
  }

  if (maxLambda > vertexPickThreshold) {
    result.elementType = MeshElement::VERTEX;
    result.index = static_cast<int64_t>(nearestVertexIdx);
  } else {
    result.elementType = MeshElement::FACE;
    result.index = static_cast<int64_t>(localInd);
  }

  return result;
}

void SimpleTriangleMesh::buildPickUI(const PickResult& rawResult) {
  SimpleTriangleMeshPickResult result = interpretPickResult(rawResult);

  if (result.index < 0) return;

  if (result.elementType == MeshElement::VERTEX) {
    ImGui::TextUnformatted(("Vertex #" + std::to_string(result.index)).c_str());

    ImGui::Spacing();
    ImGui::Indent(20.f);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
    for (auto& x : quantities) {
      SimpleTriangleMeshQuantity* q = static_cast<SimpleTriangleMeshQuantity*>(x.second.get());
      q->buildVertexInfoGUI(static_cast<size_t>(result.index));
    }
    ImGui::Columns(1);
    ImGui::Indent(-20.f);
  } else {
    ImGui::TextUnformatted(("Face #" + std::to_string(result.index)).c_str());

    ImGui::Spacing();
    ImGui::Indent(20.f);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
    for (auto& x : quantities) {
      SimpleTriangleMeshQuantity* q = static_cast<SimpleTriangleMeshQuantity*>(x.second.get());
      q->buildFaceInfoGUI(static_cast<size_t>(result.index));
    }
    ImGui::Columns(1);
    ImGui::Indent(-20.f);
  }
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

SimpleTriangleMesh* SimpleTriangleMesh::setSelectionMode(MeshSelectionMode newMode) {
  selectionMode = newMode;
  requestRedraw();
  return this;
}
MeshSelectionMode SimpleTriangleMesh::getSelectionMode() { return selectionMode.get(); }


// === Quantity adder implementations

SimpleTriangleMeshVertexScalarQuantity*
SimpleTriangleMesh::addVertexScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SimpleTriangleMeshVertexScalarQuantity* q = new SimpleTriangleMeshVertexScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

SimpleTriangleMeshFaceScalarQuantity*
SimpleTriangleMesh::addFaceScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SimpleTriangleMeshFaceScalarQuantity* q = new SimpleTriangleMeshFaceScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

SimpleTriangleMeshVertexColorQuantity*
SimpleTriangleMesh::addVertexColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SimpleTriangleMeshVertexColorQuantity* q = new SimpleTriangleMeshVertexColorQuantity(name, colors, *this);
  addQuantity(q);
  return q;
}

SimpleTriangleMeshFaceColorQuantity*
SimpleTriangleMesh::addFaceColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors) {
  checkForQuantityWithNameAndDeleteOrError(name);
  SimpleTriangleMeshFaceColorQuantity* q = new SimpleTriangleMeshFaceColorQuantity(name, colors, *this);
  addQuantity(q);
  return q;
}


} // namespace polyscope

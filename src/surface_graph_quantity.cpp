// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_graph_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceGraphQuantity::SurfaceGraphQuantity(std::string name, std::vector<glm::vec3> nodes_,
                                           std::vector<std::array<size_t, 2>> edges_, SurfaceMesh& mesh_)
    : SurfaceMeshQuantity(name, mesh_), nodes(std::move(nodes_)), edges(std::move(edges_)),
      radius(uniquePrefix() + "#radius", 0.002), color(uniquePrefix() + "#color", getNextUniqueColor()) {
  // Validate that indices are in bounds
  for (auto& p : edges) {
    if (p[0] >= nodes.size()) {
      warning("surface graph [" + name + "] has out of bounds edge index",
              "index = " + std::to_string(p[0]) + " but nNodes = " + std::to_string(nodes.size()));
    }
    if (p[1] >= nodes.size()) {
      warning("surface graph [" + name + "] has out of bounds edge index",
              "index = " + std::to_string(p[1]) + " but nNodes = " + std::to_string(nodes.size()));
    }
  }
}

void SurfaceGraphQuantity::draw() {
  if (!isEnabled()) return;

  if (pointProgram == nullptr || lineProgram == nullptr) {
    createPrograms();
  }

  setUniforms();

  pointProgram->draw();
  lineProgram->draw();
}

void SurfaceGraphQuantity::setUniforms() {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  pointProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  lineProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  pointProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  lineProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

  // Radii and colors
  pointProgram->setUniform("u_pointRadius", getRadius());
  lineProgram->setUniform("u_radius", getRadius());
  pointProgram->setUniform("u_baseColor", getColor());
  lineProgram->setUniform("u_baseColor", getColor());

  parent.setTransformUniforms(*pointProgram);
  parent.setTransformUniforms(*lineProgram);
}


void SurfaceGraphQuantity::createPrograms() {

  { // Point program
    pointProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SHADE_BASECOLOR"});
    pointProgram->setAttribute("a_position", nodes);
    render::engine->setMaterial(*pointProgram, parent.getMaterial());
  }

  { // Line program
    lineProgram = render::engine->requestShader("RAYCAST_CYLINDER", {"SHADE_BASECOLOR"});

    // Build buffers
    std::vector<glm::vec3> edgeStarts, edgeEnds;
    edgeStarts.reserve(edges.size());
    edgeEnds.reserve(edges.size());
    for (auto& p : edges) {
      edgeStarts.push_back(nodes[p[0]]);
      edgeEnds.push_back(nodes[p[1]]);
    }

    // Store data in buffers
    lineProgram->setAttribute("a_position_tail", edgeStarts);
    lineProgram->setAttribute("a_position_tip", edgeEnds);

    render::engine->setMaterial(*lineProgram, parent.getMaterial());
  }
}

void SurfaceGraphQuantity::buildCustomUI() {
  ImGui::SameLine();
  if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) setColor(getColor());
  ImGui::Text("Nodes: %lu  Edges: %lu", nodes.size(), edges.size());
  if (ImGui::SliderFloat("Radius", radius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    radius.manuallyChanged();
    requestRedraw();
  }
}

std::string SurfaceGraphQuantity::niceName() { return name; }

void SurfaceGraphQuantity::refresh() {
  pointProgram.reset();
  lineProgram.reset();
  Quantity::refresh();
}

SurfaceGraphQuantity* SurfaceGraphQuantity::setRadius(double newVal, bool isRelative) {
  radius = ScaledValue<float>(newVal, isRelative);
  requestRedraw();
  return this;
}

double SurfaceGraphQuantity::getRadius() { return radius.get().asAbsolute(); }

SurfaceGraphQuantity* SurfaceGraphQuantity::setColor(glm::vec3 newColor) {
  color = newColor;
  requestRedraw();
  return this;
}
glm::vec3 SurfaceGraphQuantity::getColor() { return color.get(); }

} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/ribbon_artist.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

RibbonArtist::RibbonArtist(Structure& parentStructure_,
                           const std::vector<std::vector<std::array<glm::vec3, 2>>>& ribbons_, std::string uniqueName,
                           double normalOffsetFraction_)
    : parentStructure(parentStructure_), ribbons(ribbons_), normalOffsetFraction(normalOffsetFraction_),
      enabled(parentStructure.uniquePrefix() + "#ribbon#" + "uniqueName" + "#enabled", true),
      ribbonWidth(parentStructure.uniquePrefix() + "#ribbon#" + "uniqueName" + "#ribbonWidth", relativeValue(5e-4)) {
  createProgram();
}

void RibbonArtist::deleteProgram() { program.reset(); }

void RibbonArtist::createProgram() {

  // Create the program
  program = render::engine->generateShaderProgram(
      {render::RIBBON_VERT_SHADER, render::RIBBON_GEOM_SHADER, render::RIBBON_FRAG_SHADER},
      DrawMode::IndexedLineStripAdjacency);

  // Set the restart index for the line strip
  unsigned int restartInd = -1;
  program->setPrimitiveRestartIndex(restartInd);

  // Compute length scales and whatnot
  float normalOffset = static_cast<float>(state::lengthScale * normalOffsetFraction);

  // == Fill buffers

  // Trace a whole bunch of lines along the surface
  // TODO Expensive yet trivially parallelizable
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> colors;
  std::vector<unsigned int> indices;
  unsigned int nPts = 0;
  const render::ValueColorMap& cmapValue = render::engine->getColorMap(cMap);
  for (size_t iLine = 0; iLine < ribbons.size(); iLine++) {

    // Process each point from the list
    std::vector<std::array<glm::vec3, 2>> line = ribbons[iLine];

    // Offset points along normals
    for (std::array<glm::vec3, 2>& x : line) {
      x[0] += x[1] * normalOffset;
    }

    // Fill the render buffer for this line
    if (line.size() <= 1) {
      continue;
    }

    // Sample a color for this line
    glm::vec3 lineColor = cmapValue.getValue(randomUnit());

    // Add a false point at the beginning (so it's not a special case for the geometry shader)
    float EPS = 0.01;
    glm::vec3 fakeFirst = line[0][0] + EPS * (line[0][0] - line[1][0]);
    positions.push_back(fakeFirst);
    normals.push_back(line.front()[1]);
    colors.push_back(lineColor);
    indices.push_back(nPts++);

    // Add all of the real points
    for (auto& pn : line) {
      positions.push_back(pn[0]);
      normals.push_back(pn[1]);
      colors.push_back(lineColor);
      indices.push_back(nPts++);
    }

    // Add a false point at the end too
    glm::vec3 fakeLast = line.back()[0] + EPS * (line.back()[0] - line[line.size() - 2][0]);
    positions.push_back(fakeLast);
    normals.push_back(line.back()[1]);
    colors.push_back(lineColor);
    indices.push_back(nPts++);

    // Restart index allows us to draw multiple lines from a single buffer
    indices.push_back(restartInd);
  }

  // Store the values in GL buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_color", colors);
  program->setIndex(indices);

  render::engine->setMaterial(*program, "wax");
}


void RibbonArtist::draw() {

  if (!enabled.get()) {
    return;
  }

  if (!program) {
    createProgram();
  }

  // Set uniforms
  parentStructure.setTransformUniforms(*program);

  glm::vec3 eyePos = view::getCameraWorldPosition();

  program->setUniform("u_ribbonWidth", getWidth());
  program->setUniform("u_depthOffset", 1e-4);

  // Draw
  render::engine->setDepthMode(DepthMode::LEqualReadOnly);
  render::engine->setBlendMode(BlendMode::Over);

  program->draw();

  render::engine->setDepthMode();
  render::engine->setBlendMode();
}


void RibbonArtist::buildParametersGUI() {

  if (render::buildColormapSelector(cMap)) {
    deleteProgram();
  }

  ImGui::PushItemWidth(150);
  if (ImGui::SliderFloat("Ribbon width", ribbonWidth.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    ribbonWidth.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}

RibbonArtist* RibbonArtist::setEnabled(bool newEnabled) {
  enabled = newEnabled;
  requestRedraw();
  return this;
}

bool RibbonArtist::getEnabled() { return enabled.get(); }

RibbonArtist* RibbonArtist::setWidth(double newVal, bool isRelative) {
  ribbonWidth = ScaledValue<float>(newVal, isRelative);
  requestRedraw();
  return this;
}

double RibbonArtist::getWidth() { return ribbonWidth.get().asAbsolute(); }


} // namespace polyscope

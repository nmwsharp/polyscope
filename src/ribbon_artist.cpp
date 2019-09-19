// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/ribbon_artist.h"

#include "polyscope/gl/color_maps.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/ribbon_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

RibbonArtist::RibbonArtist(Structure& parentStructure_,
                           const std::vector<std::vector<std::array<glm::vec3, 2>>>& ribbons_,
                           double normalOffsetFraction_)
    : parentStructure(parentStructure_), ribbons(ribbons_), normalOffsetFraction(normalOffsetFraction_) {}

void RibbonArtist::deleteProgram() { program.reset(); }

void RibbonArtist::createProgram() {

  // Create the program
  program.reset(new gl::GLProgram(&gl::RIBBON_VERT_SHADER, &gl::RIBBON_GEOM_SHADER, &gl::RIBBON_FRAG_SHADER,
                                  gl::DrawMode::IndexedLineStripAdjacency));

  // Set the restart index for the line strip
  GLuint restartInd = static_cast<GLuint>(-1);
  program->setPrimitiveRestartIndex(restartInd);

  // Compute length scales and whatnot
  float normalOffset = static_cast<float>(state::lengthScale * normalOffsetFraction);
  ribbonWidth = 5 * 1e-4;

  // == Fill buffers

  // Trace a whole bunch of lines along the surface
  // TODO Expensive yet trivially parallelizable
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> colors;
  std::vector<unsigned int> indices;
  unsigned int nPts = 0;
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
    glm::vec3 lineColor = gl::getColorMap(cMap).getValue(randomUnit());

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

  setMaterialForProgram(*program, "wax");
}


void RibbonArtist::draw() {

  if (!enabled) {
    return;
  }

  if (!program) {
    createProgram();
  }

  // Set uniforms
  parentStructure.setTransformUniforms(*program);

  glm::vec3 eyePos = view::getCameraWorldPosition();

  program->setUniform("u_ribbonWidth", ribbonWidth * state::lengthScale);
  program->setUniform("u_depthOffset", 1e-4);

  // Draw
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDepthFunc(GL_LEQUAL);

  program->draw();

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}


void RibbonArtist::buildParametersGUI() {

  if (gl::buildColormapSelector(cMap)) {
    deleteProgram();
  }

  ImGui::PushItemWidth(150);
  ImGui::SliderFloat("Ribbon width", &ribbonWidth, 0.0, .1, "%.5f", 3.);
  ImGui::PopItemWidth();
}


} // namespace polyscope

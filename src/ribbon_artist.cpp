#include "polyscope/ribbon_artist.h"

#include "polyscope/gl/colormap_sets.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/ribbon_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

RibbonArtist::RibbonArtist(const std::vector<std::vector<std::array<Vector3, 2>>>& ribbons_,
                           double normalOffsetFraction_)
    : ribbons(ribbons_), normalOffsetFraction(normalOffsetFraction_) {}

RibbonArtist::~RibbonArtist() { deleteProgram(); }

void RibbonArtist::deleteProgram() { safeDelete(program); }

void RibbonArtist::createProgram() {

  // Create the program
  program = new gl::GLProgram(&RIBBON_VERT_SHADER, &RIBBON_GEOM_SHADER, &RIBBON_FRAG_SHADER,
                              gl::DrawMode::IndexedLineStripAdjacency);

  // Set the restart index for the line strip
  GLuint restartInd = static_cast<GLuint>(-1);
  program->setPrimitiveRestartIndex(restartInd);

  // Compute length scales and whatnot
  double normalOffset = state::lengthScale * normalOffsetFraction;
  ribbonWidth = 5 * 1e-4;

  // == Fill buffers

  // Trace a whole bunch of lines along the surface
  // TODO Expensive yet trivially parallelizable
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> colors;
  std::vector<unsigned int> indices;
  unsigned int nPts = 0;
  for (size_t iLine = 0; iLine < ribbons.size(); iLine++) {

    // Process each point from the list
    std::vector<std::array<Vector3, 2>> line = ribbons[iLine];

    // Offset points along normals
    for (std::array<Vector3, 2>& x : line) {
      x[0] += x[1] * normalOffset;
    }

    // Fill the render buffer for this line
    if (line.size() <= 1) {
      continue;
    }

    // Sample a color for this line
    Vector3 lineColor = gl::allColormaps[iColorMap]->getValue(unitRand());

    // Add a false point at the beginning (so it's not a special case for the geometry shader)
    double EPS = 0.01;
    Vector3 fakeFirst = line[0][0] + EPS * (line[0][0] - line[1][0]);
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
    Vector3 fakeLast = line.back()[0] + EPS * (line.back()[0] - line[line.size() - 2][0]);
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
}


void RibbonArtist::draw() {

  if (!enabled) {
    return;
  }

  if (!program) {
    createProgram();
  }

  // Set uniforms
  glm::mat4 modelviewMat = view::getCameraViewMatrix() * objectTransform;
  program->setUniform("u_viewMatrix", glm::value_ptr(modelviewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);


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

  int iColormapBefore = iColorMap;
  ImGui::Combo("##colormap", &iColorMap, gl::allColormapNames, IM_ARRAYSIZE(gl::allColormapNames));
  if (iColorMap != iColormapBefore) {
    deleteProgram();
  }

  ImGui::SliderFloat("Ribbon width", &ribbonWidth, 0.0, .1, "%.5f", 3.);
}


} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/parameterization_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================

SurfaceParameterizationQuantity::SurfaceParameterizationQuantity(std::string name, ParamCoordsType type_,
                                                                 SurfaceMesh& mesh_)
    : SurfaceMeshQuantity(name, mesh_, true), coordsType(type_) {

  // Set default colormap
  cMap = gl::ColorMapID::PHASE;

  // Set a checker color
  checkColor1 = gl::RGB_PINK;
  glm::vec3 checkColor1HSV = RGBtoHSV(checkColor1);
  checkColor1HSV.y *= 0.15; // very light
  checkColor2 = HSVtoRGB(checkColor1HSV);

  // set grid color
  gridLineColor = gl::RGB_WHITE;
  gridBackgroundColor = gl::RGB_PINK;
}

void SurfaceParameterizationQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  setProgramUniforms(*program);

  program->draw();
}

void SurfaceParameterizationQuantity::createProgram() {
  // Create the program to draw this quantity

  switch (vizStyle) {
  case ParamVizStyle::CHECKER:
    program.reset(new gl::GLProgram(&gl::PARAM_SURFACE_VERT_SHADER, &gl::PARAM_CHECKER_SURFACE_FRAG_SHADER,
                                    gl::DrawMode::Triangles));
    break;
  case ParamVizStyle::GRID:
    program.reset(new gl::GLProgram(&gl::PARAM_SURFACE_VERT_SHADER, &gl::PARAM_GRID_SURFACE_FRAG_SHADER,
                                    gl::DrawMode::Triangles));
    break;
  case ParamVizStyle::LOCAL_CHECK:
    program.reset(new gl::GLProgram(&gl::PARAM_SURFACE_VERT_SHADER, &gl::PARAM_LOCAL_CHECKER_SURFACE_FRAG_SHADER,
                                    gl::DrawMode::Triangles));
    program->setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
    break;
  case ParamVizStyle::LOCAL_RAD:
    program.reset(new gl::GLProgram(&gl::PARAM_SURFACE_VERT_SHADER, &gl::PARAM_LOCAL_RAD_SURFACE_FRAG_SHADER,
                                    gl::DrawMode::Triangles));
    program->setTextureFromColormap("t_colormap", gl::getColorMap(cMap));
    break;
  }

  // Fill color buffers
  fillColorBuffers(*program);
  parent.fillGeometryBuffers(*program);

  setMaterialForProgram(*program, "wax");
}


// Update range uniforms
void SurfaceParameterizationQuantity::setProgramUniforms(gl::GLProgram& program) {

  // Interpretatin of modulo parameter depends on data type
  switch (coordsType) {
  case ParamCoordsType::UNIT:
    program.setUniform("u_modLen", modLen);
    break;
  case ParamCoordsType::WORLD:
    program.setUniform("u_modLen", modLen * state::lengthScale);
    break;
  }

  // Set other uniforms needed
  switch (vizStyle) {
  case ParamVizStyle::CHECKER:
    program.setUniform("u_color1", checkColor1);
    program.setUniform("u_color2", checkColor2);
    break;
  case ParamVizStyle::GRID:
    program.setUniform("u_gridLineColor", gridLineColor);
    program.setUniform("u_gridBackgroundColor", gridBackgroundColor);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD:
    program.setUniform("u_angle", localRot);
    break;
  }
}

namespace {
// Helper to name styles
std::string styleName(ParamVizStyle v) {
  switch (v) {
  case ParamVizStyle::CHECKER:
    return "checker";
    break;
  case ParamVizStyle::GRID:
    return "grid";
    break;
  case ParamVizStyle::LOCAL_CHECK:
    return "local grid";
    break;
  case ParamVizStyle::LOCAL_RAD:
    return "local dist";
    break;
  }
  throw std::runtime_error("broken");
}

} // namespace

void SurfaceParameterizationQuantity::buildCustomUI() {
  ImGui::PushItemWidth(100);

  ImGui::SameLine(); // put it next to enabled

  // Choose viz style
  if (ImGui::BeginCombo("style", styleName(vizStyle).c_str())) {
    for (ParamVizStyle s :
         {ParamVizStyle::CHECKER, ParamVizStyle::GRID, ParamVizStyle::LOCAL_CHECK, ParamVizStyle::LOCAL_RAD}) {
      if (ImGui::Selectable(styleName(s).c_str(), s == vizStyle)) {
        setStyle(s);
      }
    }
    ImGui::EndCombo();
  }


  // Modulo stripey width
  ImGui::DragFloat("period", &modLen, .001, 0.0001, 1.0, "%.4f", 2.0);


  ImGui::PopItemWidth();

  switch (vizStyle) {
  case ParamVizStyle::CHECKER:
    ImGui::ColorEdit3("##colors2", (float*)&checkColor1, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit3("colors", (float*)&checkColor2, ImGuiColorEditFlags_NoInputs);
    break;
  case ParamVizStyle::GRID:
    ImGui::ColorEdit3("base", (float*)&gridBackgroundColor, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit3("line", (float*)&gridLineColor, ImGuiColorEditFlags_NoInputs);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD: {
    // Angle slider
    ImGui::PushItemWidth(100);
    ImGui::SliderAngle("angle shift", &localRot, -180, 180); // displays in degrees, works in radians
    ImGui::PopItemWidth();

    // Set colormap
    if(buildColormapSelector(cMap)) {
      program.reset();
    }
  }

  break;
  }
}


SurfaceParameterizationQuantity* SurfaceParameterizationQuantity::setStyle(ParamVizStyle newStyle) {
  vizStyle = newStyle;
  program.reset();
  requestRedraw();
  return this;
}

void SurfaceParameterizationQuantity::geometryChanged() { program.reset(); }

// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================


SurfaceCornerParameterizationQuantity::SurfaceCornerParameterizationQuantity(std::string name,
                                                                             std::vector<glm::vec2> coords_,
                                                                             ParamCoordsType type_, SurfaceMesh& mesh_)
    : SurfaceParameterizationQuantity(name, type_, mesh_), coords(std::move(coords_)) {}

std::string SurfaceCornerParameterizationQuantity::niceName() { return name + " (corner parameterization)"; }


void SurfaceCornerParameterizationQuantity::fillColorBuffers(gl::GLProgram& p) {

  std::vector<glm::vec2> coordVal;
  coordVal.reserve(3 * parent.nFacesTriangulation());

  size_t cornerCount = 0;
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t cRoot = cornerCount;
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t cB = cornerCount + j;
      size_t cC = cornerCount + ((j + 1) % D);

      coordVal.push_back(coords[cRoot]);
      coordVal.push_back(coords[cB]);
      coordVal.push_back(coords[cC]);
    }

    cornerCount += D;
  }

  // Store data in buffers
  p.setAttribute("a_coord", coordVal);
}

void SurfaceCornerParameterizationQuantity::buildHalfedgeInfoGUI(size_t heInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coords[heInd].x, coords[heInd].y);
  ImGui::NextColumn();
}


// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================


SurfaceVertexParameterizationQuantity::SurfaceVertexParameterizationQuantity(std::string name,
                                                                             std::vector<glm::vec2> coords_,
                                                                             ParamCoordsType type_, SurfaceMesh& mesh_)
    : SurfaceParameterizationQuantity(name, type_, mesh_), coords(std::move(coords_)) {}

std::string SurfaceVertexParameterizationQuantity::niceName() { return name + " (vertex parameterization)"; }

void SurfaceVertexParameterizationQuantity::fillColorBuffers(gl::GLProgram& p) {

  std::vector<glm::vec2> coordVal;
  coordVal.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      size_t vC = face[(j + 1) % D];

      coordVal.push_back(coords[vRoot]);
      coordVal.push_back(coords[vB]);
      coordVal.push_back(coords[vC]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_coord", coordVal);
}

void SurfaceVertexParameterizationQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coords[vInd].x, coords[vInd].y);
  ImGui::NextColumn();
}


} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================

SurfaceParameterizationQuantity::SurfaceParameterizationQuantity(std::string name, ParamCoordsType type_,
                                                                 ParamVizStyle style_, SurfaceMesh& mesh_)
    : SurfaceMeshQuantity(name, mesh_, true), coordsType(type_), checkerSize(uniquePrefix() + "#checkerSize", 0.02),
      vizStyle(uniquePrefix() + "#vizStyle", style_), checkColor1(uniquePrefix() + "#checkColor1", render::RGB_PINK),
      checkColor2(uniquePrefix() + "#checkColor2", glm::vec3(.976, .856, .885)),
      gridLineColor(uniquePrefix() + "#gridLineColor", render::RGB_WHITE),
      gridBackgroundColor(uniquePrefix() + "#gridBackgroundColor", render::RGB_PINK),
      altDarkness(uniquePrefix() + "#altDarkness", 0.5), cMap(uniquePrefix() + "#cMap", "phase")

{}

void SurfaceParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  setProgramUniforms(*program);
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);

  program->draw();
}

void SurfaceParameterizationQuantity::createProgram() {
  // Create the program to draw this quantity

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    // program = render::engine->generateShaderProgram(
    //{render::PARAM_SURFACE_VERT_SHADER, render::PARAM_CHECKER_SURFACE_FRAG_SHADER}, DrawMode::Triangles);
    program = render::engine->requestShader(
        "MESH", parent.addSurfaceMeshRules({"MESH_PROPAGATE_VALUE2", "SHADE_CHECKER_VALUE2"}));
    break;
  case ParamVizStyle::GRID:
    // program = render::engine->generateShaderProgram(
    //{render::PARAM_SURFACE_VERT_SHADER, render::PARAM_GRID_SURFACE_FRAG_SHADER}, DrawMode::Triangles);
    program = render::engine->requestShader("MESH",
                                            parent.addSurfaceMeshRules({"MESH_PROPAGATE_VALUE2", "SHADE_GRID_VALUE2"}));
    break;
  case ParamVizStyle::LOCAL_CHECK:
    // program = render::engine->generateShaderProgram(
    //{render::PARAM_SURFACE_VERT_SHADER, render::PARAM_LOCAL_CHECKER_SURFACE_FRAG_SHADER}, DrawMode::Triangles);
    program = render::engine->requestShader(
        "MESH",
        parent.addSurfaceMeshRules({"MESH_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2", "CHECKER_VALUE2COLOR"}));
    program->setTextureFromColormap("t_colormap", cMap.get());
    break;
  case ParamVizStyle::LOCAL_RAD:
    // program = render::engine->generateShaderProgram(
    //{render::PARAM_SURFACE_VERT_SHADER, render::PARAM_LOCAL_RAD_SURFACE_FRAG_SHADER}, DrawMode::Triangles);
    program = render::engine->requestShader(
        "MESH", parent.addSurfaceMeshRules({"MESH_PROPAGATE_VALUE2", "SHADE_COLORMAP_ANGULAR2", "SHADEVALUE_MAG_VALUE2",
                                            "ISOLINE_STRIPE_VALUECOLOR"}));
    program->setTextureFromColormap("t_colormap", cMap.get());
    break;
  }

  // Fill color buffers
  fillColorBuffers(*program);
  parent.fillGeometryBuffers(*program);

  render::engine->setMaterial(*program, parent.getMaterial());
}


// Update range uniforms
void SurfaceParameterizationQuantity::setProgramUniforms(render::ShaderProgram& program) {
  // Interpretatin of modulo parameter depends on data type
  switch (coordsType) {
  case ParamCoordsType::UNIT:
    program.setUniform("u_modLen", getCheckerSize());
    break;
  case ParamCoordsType::WORLD:
    program.setUniform("u_modLen", getCheckerSize() * state::lengthScale);
    break;
  }

  // Set other uniforms needed
  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    program.setUniform("u_color1", getCheckerColors().first);
    program.setUniform("u_color2", getCheckerColors().second);
    break;
  case ParamVizStyle::GRID:
    program.setUniform("u_gridLineColor", getGridColors().first);
    program.setUniform("u_gridBackgroundColor", getGridColors().second);
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD:
    program.setUniform("u_angle", localRot);
    program.setUniform("u_modDarkness", getAltDarkness());
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
  if (ImGui::BeginCombo("style", styleName(getStyle()).c_str())) {
    for (ParamVizStyle s :
         {ParamVizStyle::CHECKER, ParamVizStyle::GRID, ParamVizStyle::LOCAL_CHECK, ParamVizStyle::LOCAL_RAD}) {
      if (ImGui::Selectable(styleName(s).c_str(), s == getStyle())) {
        setStyle(s);
      }
    }
    ImGui::EndCombo();
  }


  // Modulo stripey width
  if (ImGui::DragFloat("period", &checkerSize.get(), .001, 0.0001, 1.0, "%.4f", 2.0)) {
    setCheckerSize(getCheckerSize());
  }


  ImGui::PopItemWidth();

  switch (getStyle()) {
  case ParamVizStyle::CHECKER:
    if (ImGui::ColorEdit3("##colors2", &checkColor1.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("colors", &checkColor2.get()[0], ImGuiColorEditFlags_NoInputs))
      setCheckerColors(getCheckerColors());
    break;
  case ParamVizStyle::GRID:
    if (ImGui::ColorEdit3("base", &gridBackgroundColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    ImGui::SameLine();
    if (ImGui::ColorEdit3("line", &gridLineColor.get()[0], ImGuiColorEditFlags_NoInputs))
      setGridColors(getGridColors());
    break;
  case ParamVizStyle::LOCAL_CHECK:
  case ParamVizStyle::LOCAL_RAD: {
    // Angle slider
    ImGui::PushItemWidth(100);
    ImGui::SliderAngle("angle shift", &localRot, -180,
                       180); // displays in degrees, works in radians TODO refresh/update/persist
    if (ImGui::DragFloat("alt darkness", &altDarkness.get(), 0.01, 0., 1.)) {
      altDarkness.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();

    // Set colormap
    if (render::buildColormapSelector(cMap.get())) {
      setColorMap(getColorMap());
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

ParamVizStyle SurfaceParameterizationQuantity::getStyle() { return vizStyle.get(); }

SurfaceParameterizationQuantity*
SurfaceParameterizationQuantity::setCheckerColors(std::pair<glm::vec3, glm::vec3> colors) {
  checkColor1 = colors.first;
  checkColor2 = colors.second;
  requestRedraw();
  return this;
}

std::pair<glm::vec3, glm::vec3> SurfaceParameterizationQuantity::getCheckerColors() {
  return std::make_pair(checkColor1.get(), checkColor2.get());
}

SurfaceParameterizationQuantity*
SurfaceParameterizationQuantity::setGridColors(std::pair<glm::vec3, glm::vec3> colors) {
  gridLineColor = colors.first;
  gridBackgroundColor = colors.second;
  requestRedraw();
  return this;
}

std::pair<glm::vec3, glm::vec3> SurfaceParameterizationQuantity::getGridColors() {
  return std::make_pair(gridLineColor.get(), gridBackgroundColor.get());
}

SurfaceParameterizationQuantity* SurfaceParameterizationQuantity::setCheckerSize(double newVal) {
  checkerSize = newVal;
  requestRedraw();
  return this;
}

double SurfaceParameterizationQuantity::getCheckerSize() { return checkerSize.get(); }

SurfaceParameterizationQuantity* SurfaceParameterizationQuantity::setColorMap(std::string name) {
  cMap = name;
  program.reset();
  requestRedraw();
  return this;
}
std::string SurfaceParameterizationQuantity::getColorMap() { return cMap.get(); }

SurfaceParameterizationQuantity* SurfaceParameterizationQuantity::setAltDarkness(double newVal) {
  altDarkness = newVal;
  requestRedraw();
  return this;
}

double SurfaceParameterizationQuantity::getAltDarkness() { return altDarkness.get(); }

void SurfaceParameterizationQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================


SurfaceCornerParameterizationQuantity::SurfaceCornerParameterizationQuantity(std::string name,
                                                                             std::vector<glm::vec2> coords_,
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_, SurfaceMesh& mesh_)
    : SurfaceParameterizationQuantity(name, type_, style_, mesh_), coords(std::move(coords_)) {}

std::string SurfaceCornerParameterizationQuantity::niceName() { return name + " (corner parameterization)"; }


void SurfaceCornerParameterizationQuantity::fillColorBuffers(render::ShaderProgram& p) {
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
  p.setAttribute("a_value2", coordVal);
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
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_, SurfaceMesh& mesh_)
    : SurfaceParameterizationQuantity(name, type_, style_, mesh_), coords(std::move(coords_)) {}

std::string SurfaceVertexParameterizationQuantity::niceName() { return name + " (vertex parameterization)"; }

void SurfaceVertexParameterizationQuantity::fillColorBuffers(render::ShaderProgram& p) {
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
  p.setAttribute("a_value2", coordVal);
}

void SurfaceVertexParameterizationQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coords[vInd].x, coords[vInd].y);
  ImGui::NextColumn();
}


} // namespace polyscope

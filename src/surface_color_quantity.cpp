// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceColorQuantity::SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                           const std::vector<glm::vec3>& colorValues_)
    : SurfaceMeshQuantity(name, mesh_, true), ColorQuantity(*this, colorValues_), definedOn(definedOn_) {}

void SurfaceColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  program->draw();
}

void SurfaceColorQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildColorOptionsUI();

    ImGui::EndPopup();
  }

  buildColorUI();
}


// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

SurfaceVertexColorQuantity::SurfaceVertexColorQuantity(std::string name, SurfaceMesh& mesh_,
                                                       std::vector<glm::vec3> colorValues_)
    : SurfaceColorQuantity(name, mesh_, "vertex", colorValues_)

{}

void SurfaceVertexColorQuantity::createProgram() {
  // Create the program to draw this quantity
  // clang-format off
  program = render::engine->requestShader("MESH", 
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSurfaceMeshRules(
            {"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}
          )
        )
      )
    );
  // clang-format on

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceVertexColorQuantity::buildColorOptionsUI() {
  ColorQuantity::buildColorOptionsUI();
  ImGui::TextUnformatted("(no options available)"); // remove once there is something in this menu
}

void SurfaceVertexColorQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(vInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string SurfaceColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void SurfaceColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ========================================================
// ==========            Face Color              ==========
// ========================================================

SurfaceFaceColorQuantity::SurfaceFaceColorQuantity(std::string name, SurfaceMesh& mesh_,
                                                   std::vector<glm::vec3> colorValues_)
    : SurfaceColorQuantity(name, mesh_, "face", colorValues_)

{}

void SurfaceFaceColorQuantity::createProgram() {
  // Create the program to draw this quantity
  // clang-format off
  program = render::engine->requestShader("MESH", 
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSurfaceMeshRules(
            {"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}
          )
        )
      )
    );
  // clang-format on

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceFaceColorQuantity::buildColorOptionsUI() {
  ColorQuantity::buildColorOptionsUI();
  ImGui::TextUnformatted("(no options available)"); // remove once there is something in this menu
}

void SurfaceFaceColorQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(fInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << tempColor;
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

// ========================================================
// ==========         Texture Color              ==========
// ========================================================

SurfaceTextureColorQuantity::SurfaceTextureColorQuantity(std::string name, SurfaceMesh& mesh_,
                                                         SurfaceParameterizationQuantity& param_, size_t dimX_,
                                                         size_t dimY_, std::vector<glm::vec3> colorValues_,
                                                         ImageOrigin origin_)
    : SurfaceColorQuantity(name, mesh_, "texture", colorValues_), TextureMapQuantity(*this, dimX_, dimY_, origin_),
      param(param_) {
  colors.setTextureSize(dimX, dimY);
}

void SurfaceTextureColorQuantity::createProgram() {
  // Create the program to draw this quantity
  // clang-format off
  program = render::engine->requestShader("MESH", 
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSurfaceMeshRules(
            {"MESH_PROPAGATE_TCOORD", getImageOriginRule(imageOrigin), "TEXTURE_PROPAGATE_COLOR", "SHADE_COLOR"}
          )
        )
      )
    );
  // clang-format on

  parent.setMeshGeometryAttributes(*program);

  // the indexing into the parameterization varies based on whether it is a corner or vertex quantity
  switch (param.definedOn) {
  case MeshElement::VERTEX:
    program->setAttribute("a_tCoord", param.coords.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
    break;
  case MeshElement::CORNER:
    program->setAttribute("a_tCoord", param.coords.getIndexedRenderAttributeBuffer(parent.triangleCornerInds));
    break;
  default:
    // nothing
    break;
  }

  program->setTextureFromBuffer("t_color", colors.getRenderTextureBuffer().get());
  render::engine->setMaterial(*program, parent.getMaterial());

  colors.getRenderTextureBuffer()->setFilterMode(filterMode.get());
}

void SurfaceTextureColorQuantity::buildColorOptionsUI() {
  ColorQuantity::buildColorOptionsUI();
  buildTextureMapOptionsUI();
}


} // namespace polyscope

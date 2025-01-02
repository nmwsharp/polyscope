// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                             const std::vector<float>& values_, DataType dataType_)
    : SurfaceMeshQuantity(name, mesh_, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

void SurfaceScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);
  setScalarUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  program->draw();
}


void SurfaceScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();
    buildSurfaceScalarOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
}

void SurfaceScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SurfaceScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

SurfaceVertexScalarQuantity::SurfaceVertexScalarQuantity(std::string name, const std::vector<float>& values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);
}

void SurfaceVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  if (dataType == DataType::CATEGORICAL) {
    // special case: nearst-vertex interpolation for categorical data
    // (plus some special-case handling for cases where two vertices have the same value)

    // clang-format off
    program = render::engine->requestShader("MESH",
        render::engine->addMaterialRules(parent.getMaterial(),
          parent.addSurfaceMeshRules(
            addScalarRules(
              {"MESH_PROPAGATE_VALUE_CORNER_NEAREST"}
            )
          )
        )
      );
    // clang-format on

    program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllVertexInds));

  } else {
    // common case: linear interpolation within each triangle

    // clang-format off
    program = render::engine->requestShader("MESH",
        render::engine->addMaterialRules(parent.getMaterial(),
          parent.addSurfaceMeshRules(
            addScalarRules(
              {"MESH_PROPAGATE_VALUE"}
            )
          )
        )
      );
    // clang-format on

    program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  }

  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

std::shared_ptr<render::AttributeBuffer> SurfaceVertexScalarQuantity::getAttributeBuffer() {
  return values.getIndexedRenderAttributeBuffer(parent.triangleVertexInds);
}

void SurfaceVertexScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(vInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceFaceScalarQuantity::SurfaceFaceScalarQuantity(std::string name, const std::vector<float>& values_,
                                                     SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "face", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  parent.faceAreas.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);
}

void SurfaceFaceScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  // clang-format off
  program = render::engine->requestShader("MESH",
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addScalarRules(
            {"MESH_PROPAGATE_VALUE"}
          )
        )
      )
    );
  // clang-format on

  program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


std::shared_ptr<render::AttributeBuffer> SurfaceFaceScalarQuantity::getAttributeBuffer() {
  return values.getIndexedRenderAttributeBuffer(parent.triangleFaceInds);
}

void SurfaceFaceScalarQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(fInd));
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

// TODO need to do something about values for internal edges in triangulated polygons

SurfaceEdgeScalarQuantity::SurfaceEdgeScalarQuantity(std::string name, const std::vector<float>& values_,
                                                     SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);
}

void SurfaceEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  // clang-format off
  program = render::engine->requestShader("MESH",
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addScalarRules(
            {"MESH_PROPAGATE_HALFEDGE_VALUE"}
          )
        )
      )
    );
  // clang-format on

  program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllEdgeInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

std::shared_ptr<render::AttributeBuffer> SurfaceEdgeScalarQuantity::getAttributeBuffer() {
  return values.getIndexedRenderAttributeBuffer(parent.triangleAllEdgeInds);
}

void SurfaceEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(eInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

SurfaceHalfedgeScalarQuantity::SurfaceHalfedgeScalarQuantity(std::string name, const std::vector<float>& values_,
                                                             SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "halfedge", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);
}

void SurfaceHalfedgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  // clang-format off
  program = render::engine->requestShader("MESH",
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addScalarRules(
            {"MESH_PROPAGATE_HALFEDGE_VALUE"}
          )
        )
      )
    );
  // clang-format on

  program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllHalfedgeInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

std::shared_ptr<render::AttributeBuffer> SurfaceHalfedgeScalarQuantity::getAttributeBuffer() {
  return values.getIndexedRenderAttributeBuffer(parent.triangleAllHalfedgeInds);
}

void SurfaceHalfedgeScalarQuantity::buildHalfedgeInfoGUI(size_t heInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(heInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========          Corner Scalar           ==========
// ========================================================

SurfaceCornerScalarQuantity::SurfaceCornerScalarQuantity(std::string name, const std::vector<float>& values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "corner", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);
}

void SurfaceCornerScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  if (dataType == DataType::CATEGORICAL) {
    // special case: nearst-vertex interpolation for categorical data
    // (plus some special-case handling for cases where two vertices have the same value)

    // clang-format off
    program = render::engine->requestShader("MESH",
        render::engine->addMaterialRules(parent.getMaterial(),
          parent.addSurfaceMeshRules(
            addScalarRules(
              {"MESH_PROPAGATE_VALUE_CORNER_NEAREST"}
            )
          )
        )
      );
    // clang-format on

    program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllCornerInds));

  } else {

    // clang-format off
  program = render::engine->requestShader("MESH",
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addScalarRules(
            {"MESH_PROPAGATE_VALUE"}
          )
        )
      )
    );
    // clang-format on

    program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleCornerInds));
  }

  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

std::shared_ptr<render::AttributeBuffer> SurfaceCornerScalarQuantity::getAttributeBuffer() {
  return values.getIndexedRenderAttributeBuffer(parent.triangleCornerInds);
}

void SurfaceCornerScalarQuantity::buildCornerInfoGUI(size_t cInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(cInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========          Texture Scalar            ==========
// ========================================================

SurfaceTextureScalarQuantity::SurfaceTextureScalarQuantity(std::string name, SurfaceMesh& mesh_,
                                                           SurfaceParameterizationQuantity& param_, size_t dimX_,
                                                           size_t dimY_, const std::vector<float>& values_,
                                                           ImageOrigin origin_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", values_, dataType_),
      TextureMapQuantity(*this, dimX_, dimY_, origin_), param(param_) {
  values.setTextureSize(dimX, dimY);
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data, dataType);

  if (dataType == DataType::CATEGORICAL) {
    // default to nearest filtering for categorical data, it's probably what the user wants
    filterMode.setPassive(FilterMode::Nearest);
  }
}

void SurfaceTextureScalarQuantity::createProgram() {
  // Create the program to draw this quantity

  // clang-format off
  program = render::engine->requestShader("MESH",
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addScalarRules(
            {"MESH_PROPAGATE_TCOORD", getImageOriginRule(imageOrigin), "TEXTURE_PROPAGATE_VALUE"}
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

  program->setTextureFromBuffer("t_scalar", values.getRenderTextureBuffer().get());
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());

  values.getRenderTextureBuffer()->setFilterMode(filterMode.get());
}

void SurfaceTextureScalarQuantity::buildSurfaceScalarOptionsUI() { buildTextureMapOptionsUI(); }

std::shared_ptr<render::AttributeBuffer> SurfaceTextureScalarQuantity::getAttributeBuffer() {
  exception("unsupported operation -- cannot get attribute buffer for texture scalar quantity [" + this->name + "]");
  return std::shared_ptr<render::AttributeBuffer>(nullptr);
}

} // namespace polyscope

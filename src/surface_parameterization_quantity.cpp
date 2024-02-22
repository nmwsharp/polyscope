// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_parameterization_quantity.h"

#include <map>
#include <set>

#include "polyscope/curve_network.h"
#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================

SurfaceParameterizationQuantity::SurfaceParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                 const std::vector<glm::vec2>& coords_,
                                                                 MeshElement definedOn_, ParamCoordsType type_,
                                                                 ParamVizStyle style_)
    : SurfaceMeshQuantity(name, mesh_, true), ParameterizationQuantity(*this, coords_, type_, style_),
      definedOn(definedOn_) {

  // sanity check, this should basically never happen, but this guards against weird edge cases such
  // as persistent values restoring the style, device updates, etc
  if (getStyle() == ParamVizStyle::CHECKER_ISLANDS && !haveIslandLabels()) {
    setStyle(ParamVizStyle::CHECKER);
  }
}

void SurfaceParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  setParameterizationUniforms(*program);
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  program->draw();
}

void SurfaceParameterizationQuantity::createProgram() {

  // sanity check, this should basically never happen, but this guards against weird edge cases such
  // as persistent values restoring the style, device updates, etc
  if (getStyle() == ParamVizStyle::CHECKER_ISLANDS && !haveIslandLabels()) {
    setStyle(ParamVizStyle::CHECKER);
  }

  // Create the program to draw this quantity
  // clang-format off
  program = render::engine->requestShader("MESH", 
      render::engine->addMaterialRules(parent.getMaterial(),
        parent.addSurfaceMeshRules(
          addParameterizationRules({
            "MESH_PROPAGATE_VALUE2",
            getStyle() == ParamVizStyle::CHECKER_ISLANDS ? "MESH_PROPAGATE_FLAT_VALUE" : "",
          })
        )
      )
    );
  //

  // Fill buffers
  fillCoordBuffers(*program);
  fillParameterizationBuffers(*program);
  parent.setMeshGeometryAttributes(*program);

  if(getStyle() == ParamVizStyle::CHECKER_ISLANDS) {
    program->setAttribute("a_value", islandLabels.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
  }

  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceParameterizationQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildParameterizationOptionsUI();
    
    if (ImGui::MenuItem("Create curve network from seams")) createCurveNetworkFromSeams();

    ImGui::EndPopup();
  }

  buildParameterizationUI();
}

CurveNetwork* SurfaceParameterizationQuantity::createCurveNetworkFromSeams(std::string structureName) {
  
  // set the name to default
  if (structureName == "") {
    structureName = parent.name + " - " + name + " - seams";
  }

  // Populate data on the host
  coords.ensureHostBufferPopulated();
  parent.triangleCornerInds.ensureHostBufferPopulated();
  parent.triangleVertexInds.ensureHostBufferPopulated();
  parent.edgeIsReal.ensureHostBufferPopulated();
  parent.vertexPositions.ensureHostBufferPopulated();

  // helper to canonicalize edge direction
  auto canonicalizeEdge = [](std::pair<int32_t, int32_t>& inds, std::pair<glm::vec2, glm::vec2>& coords) 
  {
    if(inds.first > inds.second) {
      std::swap(inds.first, inds.second);
      std::swap(coords.first, coords.second);
    }
  };

  // map to find matching & not-matching edges
  // TODO set up combining hash to use unordered_map/set instead
  std::map<std::pair<int32_t, int32_t>, std::pair<glm::vec2, glm::vec2>> edgeCoords;
  std::map<std::pair<int32_t, int32_t>, int32_t> edgeCount;
  std::set<std::pair<int32_t, int32_t>> seamEdges;

  // loop over all edges
  for(size_t iT = 0; iT <  parent.nFacesTriangulation(); iT++) {
    for(size_t k = 0; k < 3; k++) {
      if(parent.edgeIsReal.data[3*iT][k] == 0.) continue; // skip internal tesselation edges

      // gather data for the edge
      int32_t iV_tail = parent.triangleVertexInds.data[3*iT + (k+0)%3];
      int32_t iV_tip = parent.triangleVertexInds.data[3*iT + (k+1)%3];
      int32_t iC_tail = parent.triangleCornerInds.data[3*iT + (k+0)%3];
      int32_t iC_tip = parent.triangleCornerInds.data[3*iT + (k+1)%3];
      std::pair<int32_t, int32_t> eInd (iV_tail, iV_tip);
      std::pair<glm::vec2, glm::vec2> eC (coords.data[iC_tail], coords.data[iC_tip]);
      canonicalizeEdge(eInd, eC); // make sure ordering is consistent

      // increment the count
      if(edgeCount.find(eInd) == edgeCount.end())  {
        edgeCount[eInd] = 1;
      } else {
        edgeCount[eInd] ++;
      }

      // check for a collision against a previously seen copy of this edge
      if(edgeCoords.find(eInd) == edgeCoords.end()) { 
        edgeCoords[eInd] = eC;
      } else {
        if( edgeCoords[eInd] != eC) {
          // it's different! mark the seam
          seamEdges.emplace(eInd);
        }
      }
    }
  }

  // add all edges that appeared any number of times other than 2
  // (boundaries and nonmanifold edges are always seams)
  for(auto& entry : edgeCount)  {
    if(entry.second != 2) {
      seamEdges.emplace(entry.first);
    }
  }

  // densely enumerate the nodes of the seam curves
  std::vector<std::array<int32_t, 2>> seamEdgeInds;
  std::map<int32_t, int32_t> vertexIndToDense;
  std::vector<glm::vec3> seamEdgeNodes;
  for(const std::pair<int32_t, int32_t>& edge: seamEdges)  {
    int32_t vA = edge.first;
    int32_t vB = edge.second;
 
    // get unique vertices for the edges
    if(vertexIndToDense.find(vA) == vertexIndToDense.end()) {
      vertexIndToDense[vA] = seamEdgeNodes.size();
      seamEdgeNodes.push_back(parent.vertexPositions.data[vA]);
    }
    int32_t nA = vertexIndToDense[vA];
    if(vertexIndToDense.find(vB) == vertexIndToDense.end()) {
      vertexIndToDense[vB] = seamEdgeNodes.size();
      seamEdgeNodes.push_back(parent.vertexPositions.data[vB]);
    }
    int32_t nB = vertexIndToDense[vB];

    // add the edge
    seamEdgeInds.push_back({nA, nB});
  }

  // add the curve network

  return registerCurveNetwork(structureName, seamEdgeNodes, seamEdgeInds);
}

size_t SurfaceParameterizationQuantity::nFaces() {
  return parent.nFaces();
}

void SurfaceParameterizationQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================


SurfaceCornerParameterizationQuantity::SurfaceCornerParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                             const std::vector<glm::vec2>& coords_,
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_)
    : SurfaceParameterizationQuantity(name, mesh_, coords_, MeshElement::CORNER, type_, style_) {}

std::string SurfaceCornerParameterizationQuantity::niceName() { return name + " (corner parameterization)"; }


void SurfaceCornerParameterizationQuantity::fillCoordBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_value2", coords.getIndexedRenderAttributeBuffer(parent.triangleCornerInds));
}

void SurfaceCornerParameterizationQuantity::buildCornerInfoGUI(size_t cInd) {

  glm::vec2 coord = coords.getValue(cInd);

  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coord.x, coord.y);
  ImGui::NextColumn();
}

// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================


SurfaceVertexParameterizationQuantity::SurfaceVertexParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                             const std::vector<glm::vec2>& coords_,
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_)
    : SurfaceParameterizationQuantity(name, mesh_, coords_, MeshElement::VERTEX, type_, style_) {}

std::string SurfaceVertexParameterizationQuantity::niceName() { return name + " (vertex parameterization)"; }

void SurfaceVertexParameterizationQuantity::fillCoordBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_value2", coords.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
}

void SurfaceVertexParameterizationQuantity::buildVertexInfoGUI(size_t vInd) {

  glm::vec2 coord = coords.getValue(vInd);

  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coord.x, coord.y);
  ImGui::NextColumn();
}


} // namespace polyscope

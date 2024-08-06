// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_custom_shader_quantity.h"

#include "polyscope/custom_shader_quantity.h"


#include "polyscope/surface_scalar_quantity.h"

#include "imgui.h"

namespace polyscope {

SurfaceCustomShaderQuantity::SurfaceCustomShaderQuantity(std::string name, SurfaceMesh& mesh_, std::string programText_)
    : SurfaceMeshQuantity(name, mesh_, true), CustomShaderQuantity(programText_) {}

void SurfaceCustomShaderQuantity::draw() {
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


void SurfaceCustomShaderQuantity::createProgram() {

  // Create the program to draw this quantity
  // clang-format off
  program = render::engine->requestShader("MESH", 
      render::engine->addMaterialRules(parent.getMaterial(),
          parent.addSurfaceMeshRules(
            {"SHADE_COLOR"}
          )
      )
    );
  // clang-format on

  parent.setMeshGeometryAttributes(*program);
  // program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}


void SurfaceCustomShaderQuantity::resolveAttribute(CustomShaderAttributeEntry& entry) {

  SurfaceMeshQuantity* quantityPtr = parent.getQuantity(entry.quantityName);
  if (quantityPtr == nullptr) {
    throw CustomShaderError("There is no quantity named " + entry.quantityName);
  }

  // Try to coerce it to one of the quantity types

  { // Vertex Scalar Quantity
    SurfaceVertexScalarQuantity* quantityTypePtr = dynamic_cast<SurfaceVertexScalarQuantity*>(quantityPtr);
    if (quantityTypePtr) {
      entry.isResolved = true;
      entry.attributeBuffer = quantityTypePtr->values.getIndexedRenderAttributeBuffer(parent.triangleVertexInds);
      entry.managedBufferWeakHandle = quantityTypePtr->values.getGenericWeakHandle();
      return;
    }
  }

  { // Face Scalar Quantity
    SurfaceFaceScalarQuantity* quantityTypePtr = dynamic_cast<SurfaceFaceScalarQuantity*>(quantityPtr);
    if (quantityTypePtr) {
      entry.isResolved = true;
      entry.attributeBuffer = quantityTypePtr->values.getIndexedRenderAttributeBuffer(parent.triangleFaceInds);
      entry.managedBufferWeakHandle = quantityTypePtr->values.getGenericWeakHandle();
      return;
    }
  }

  { // Edge Scalar Quantity
    SurfaceEdgeScalarQuantity* quantityTypePtr = dynamic_cast<SurfaceEdgeScalarQuantity*>(quantityPtr);
    if (quantityTypePtr) {
      entry.isResolved = true;
      entry.attributeBuffer = quantityTypePtr->values.getIndexedRenderAttributeBuffer(parent.triangleAllEdgeInds);
      entry.managedBufferWeakHandle = quantityTypePtr->values.getGenericWeakHandle();
      return;
    }
  }

  { // Halfedge Scalar Quantity
    SurfaceHalfedgeScalarQuantity* quantityTypePtr = dynamic_cast<SurfaceHalfedgeScalarQuantity*>(quantityPtr);
    if (quantityTypePtr) {
      entry.isResolved = true;
      entry.attributeBuffer = quantityTypePtr->values.getIndexedRenderAttributeBuffer(parent.triangleAllHalfedgeInds);
      entry.managedBufferWeakHandle = quantityTypePtr->values.getGenericWeakHandle();
      return;
    }
  }

  { // Corner Scalar Quantity
    SurfaceCornerScalarQuantity* quantityTypePtr = dynamic_cast<SurfaceCornerScalarQuantity*>(quantityPtr);
    if (quantityTypePtr) {
      entry.isResolved = true;
      entry.attributeBuffer = quantityTypePtr->values.getIndexedRenderAttributeBuffer(parent.triangleCornerInds);
      entry.managedBufferWeakHandle = quantityTypePtr->values.getGenericWeakHandle();
      return;
    }
  }

  throw CustomShaderError("There is no quantity named " + entry.quantityName);
}


std::string SurfaceCustomShaderQuantity::niceName() { return name + " (custom shader)"; }

void SurfaceCustomShaderQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

} // namespace polyscope

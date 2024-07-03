
#include "polyscope/point_cloud_tetracolor_quantity.h"
#include "polyscope/polyscope.h"

namespace polyscope {

PointCloudTetracolorQuantity::PointCloudTetracolorQuantity(std::string name, const std::vector<glm::vec4>& values_,
                                                           PointCloud& pointCloud_)
  : PointCloudQuantity(name, pointCloud_, true), TetracolorQuantity(*this, values_) {}

void PointCloudTetracolorQuantity::draw() {
  if (!isEnabled()) return;

  // Make the point program if we don't have one already
  if (pointProgram == nullptr) {
    createPointProgram();
  }

  parent.setStructureUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);
  setTetracolorUniforms(*pointProgram);

  render::engine->setBlendMode(BlendMode::Disable);
  pointProgram->draw();
}

std::string PointCloudTetracolorQuantity::getShaderNameForRenderMode() {
  if (parent.getPointRenderMode() == PointRenderMode::Sphere) {
    return "RAYCAST_SPHERE_TETRA";
  }
  else if (parent.getPointRenderMode() == PointRenderMode::Quad) {
    return "POINT_QUAD_TETRA";
  }
  return "RAYCAST_SPHERE_TETRA"; // Should never reach this.  
}

void PointCloudTetracolorQuantity::buildPickUI(size_t ind) {
  return;
}

void PointCloudTetracolorQuantity::refresh() {
  return;
}

std::string PointCloudTetracolorQuantity::niceName() {
  return name + " (tetracolor)"; 
}

void PointCloudTetracolorQuantity::createPointProgram() {
  // Create the program to draw this quantity.
  pointProgram = render::engine->requestShader( getShaderNameForRenderMode(),
    render::engine->addMaterialRules("flat_tetra",
      addTetracolorRules(
        parent.addPointCloudRules(
          {"SPHERE_PROPAGATE_TETRACOLOR", "SHADE_TETRACOLOR"}
        )
      )
    )
  );

  parent.setPointProgramGeometryAttributes(*pointProgram);
  pointProgram->setAttribute("a_tetracolor", tetracolors.getRenderAttributeBuffer());
}

} // namespace polyscope

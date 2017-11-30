#include "polyscope/surface_vector_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceVectorQuantity::SurfaceVectorQuantity(std::string name,
                                             VertexData<Vector3>& vectors_,
                                             SurfaceMesh* mesh_,
                                             VectorType vectorType_)

    : SurfaceQuantity(name, mesh_),
      vectorType(vectorType_),
      definedOn("vertex") {

  // Copy the vectors
  VertexData<Vector3> tVectors = parent->transfer.transfer(vectors_);
  for (VertexPtr v : parent->mesh->vertices()) {
    vectorRoots.push_back(parent->geometry->position(v));
    vectors.push_back(tVectors[v]);
  }

  finishConstructing();
}

SurfaceVectorQuantity::SurfaceVectorQuantity(std::string name,
                                             FaceData<Vector3>& vectors_,
                                             SurfaceMesh* mesh_,
                                             VectorType vectorType_)

    : SurfaceQuantity(name, mesh_),
      vectorType(vectorType_),
      definedOn("face") {

  // Copy the vectors
  FaceData<Vector3> tVectors = parent->transfer.transfer(vectors_);
  for (FacePtr f : parent->mesh->faces()) {
    vectorRoots.push_back(parent->geometry->barycenter(f));
    vectors.push_back(tVectors[f]);
  }

  finishConstructing();
}

void SurfaceVectorQuantity::finishConstructing() {

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<Vector3>(vectors, DataType::MAGNITUDE);
  }

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .02;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .0005;
  vectorColor = getNextPaletteColor();
}

void SurfaceVectorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) prepare();

  // Set uniforms
  glm::mat4 viewMat = view::getViewMatrix();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_radius", radiusMult * state::lengthScale);
  program->setUniform("u_color", vectorColor);

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", lengthMult * state::lengthScale);
  }

  program->draw();
}

void SurfaceVectorQuantity::prepare() {
  program = new gl::GLProgram(&PASSTHRU_VECTOR_VERT_SHADER, &VECTOR_GEOM_SHADER,
                              &SHINY_VECTOR_FRAG_SHADER, gl::DrawMode::Points);

  // Fill buffers
  std::vector<Vector3> mappedVectors;
  for (Vector3 v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", vectorRoots);
}

void SurfaceVectorQuantity::drawUI() {
  if (ImGui::TreeNode((name + " (" + definedOn + " vector)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Color", (float*)&vectorColor,
                      ImGuiColorEditFlags_NoInputs);

    // Only get to set length for non-ambient vectors
    if (vectorType != VectorType::AMBIENT) {
      ImGui::SliderFloat("Length", &lengthMult, 0.0, .1, "%.5f", 3.);
    }

    ImGui::SliderFloat("Radius", &radiusMult, 0.0, .1, "%.5f", 3.);

    {  // Draw max and min magnitude
      ImGui::TextUnformatted(mapper.printBounds().c_str());
    }

    ImGui::TreePop();
  }
}

}  // namespace polyscope

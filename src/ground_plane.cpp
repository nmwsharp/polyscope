#include "polyscope/ground_plane.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders/ground_plane_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"
#include "stb_image.h"

namespace polyscope {


bool groundPlaneEnabled = true;
float groundPlaneHeightFactor = 0.25;

// Global variables and helpers for ground plane
namespace {

bool groundPlanePrepared = false;
gl::GLProgram* groundPlaneProgram = nullptr;


void prepareGroundPlane() {

  // The program that draws the ground plane
  groundPlaneProgram = new gl::GLProgram(&GROUND_PLANE_VERT_SHADER, &GROUND_PLANE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Geometry of the ground plane, using triangles with vertices at infinity
  glm::vec4 cVert{0., 0., 0., 1.};
  glm::vec4 v1{1., 0., 0., 0.};
  glm::vec4 v2{0., 0., 1., 0.};
  glm::vec4 v3{-1., 0., 0., 0.};
  glm::vec4 v4{0., 0., -1., 0.};

  // clang-format off
  std::vector<glm::vec4> positions = {
    cVert, v2, v1,
    cVert, v3, v2,
    cVert, v4, v3,
    cVert, v1, v4
  };
  // clang-format on

  groundPlaneProgram->setAttribute("a_position", positions);


  { // Load the ground texture
    int w, h, comp;
    unsigned char* image = nullptr;
    image = stbi_load("concrete_seamless.jpg", &w, &h, &comp, STBI_rgb);
    if (image == nullptr) throw std::logic_error("Failed to load material image");
    groundPlaneProgram->setTexture2D("t_ground", image, w, h, false, false, true);
  }


  groundPlanePrepared = true;
}
};


void drawGroundPlane() {

  if (!groundPlaneEnabled) return;

  if (!groundPlanePrepared) {
    prepareGroundPlane();
  }


  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  groundPlaneProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  groundPlaneProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));
  
  glm::vec2 centerXZ{state::center.x, state::center.z};
  groundPlaneProgram->setUniform("u_centerXZ", centerXZ);

  groundPlaneProgram->setUniform("u_lengthScale", state::lengthScale);

  // Location for ground plane
  double bboxBottom = std::get<0>(state::boundingBox).y;
  double bboxHeight = std::get<1>(state::boundingBox).y - std::get<0>(state::boundingBox).y;
  double groundHeight = bboxBottom - groundPlaneHeightFactor * bboxHeight;
  groundPlaneProgram->setUniform("u_groundHeight", groundHeight);


  // Enable backface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  groundPlaneProgram->draw();


  // Disable culling afterwards, so we don't confuse ourselves
  glDisable(GL_CULL_FACE);
}

void buildGroundPlaneGui() {

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("ground plane")) {

    ImGui::Checkbox("Enabled", &groundPlaneEnabled);
  
    ImGui::SliderFloat("Height", &groundPlaneHeightFactor, -1.0, 1.0); 
  
  
  
  
    ImGui::TreePop();
  }


}

void deleteGroundPlaneResources() {

  if (!groundPlanePrepared) {
    safeDelete(groundPlaneProgram);
  }
}
};


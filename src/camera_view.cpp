#include "polyscope/camera_view.h"

#include <iterator>

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/image_shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/gl/shaders/wireframe_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include "glm/glm.hpp"

using namespace geometrycentral;

namespace polyscope {

// Initialize static member
const std::string CameraView::structureTypeName = "Camera View";
float CameraView::globalImageTransparency = 1.0;

CameraView::CameraView(std::string name, CameraParameters p_)
    : Structure(name, structureTypeName), parameters(p_) {
  prepareCameraSkeleton();
  preparePick();
}

CameraView::~CameraView() {
  if (cameraSkeletonProgram != nullptr) {
    delete cameraSkeletonProgram;
  }
  if (imageViewProgram != nullptr) {
    delete imageViewProgram;
  }
  for (auto x : images) {
    delete x.second;
  }
}

void CameraView::draw() {
  if (!enabled) {
    return;
  }

  drawWireframe();
  drawImageView();
}

void CameraView::drawWireframe() {
  if (cameraSkeletonProgram == nullptr) {
    prepareCameraSkeleton();
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  cameraSkeletonProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  cameraSkeletonProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  cameraSkeletonProgram->setUniform("u_wirecolor", gl::RGB_BLACK);

  cameraSkeletonProgram->draw();
}

void CameraView::drawImageView() {
  if (imageViewProgram == nullptr || globalImageTransparency == 0.0) {
    return;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  imageViewProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  imageViewProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  imageViewProgram->setUniform("u_transparency", globalImageTransparency);

  imageViewProgram->draw();
}

void CameraView::drawPick() {

  glDisable(GL_CULL_FACE);

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  pickProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  pickProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  pickProgram->draw();
}

// Helper function to convert glm::vec3 to Vector3
namespace {
Vector3 toV(glm::vec3 x) { return Vector3{x.x, x.y, x.z}; }
} // namespace

Vector3 CameraView::location() { return toV(parameters.getPosition()); }

void CameraView::prepare() {}

void CameraView::preparePick() {

  // Request pick index
  size_t pickInd = pick::requestPickBufferRange(this, 1);
  Vector3 indColor = pick::indToVec(pickInd);

  // Create a new pick program
  safeDelete(pickProgram);
  pickProgram = new gl::GLProgram(&FACECOLOR_PLAIN_SURFACE_VERT_SHADER, &FACECOLOR_PLAIN_SURFACE_FRAG_SHADER,
                                  gl::DrawMode::Triangles);

  // Fill an index buffer
  std::vector<Vector3> pickColors;
  std::vector<Vector3> positions;

  auto addTriangle = [&](Vector3 p1, Vector3 p2, Vector3 p3) {
    positions.push_back(p1);
    positions.push_back(p2);
    positions.push_back(p3);
    pickColors.push_back(indColor);
    pickColors.push_back(indColor);
    pickColors.push_back(indColor);
  };

  // = Build the camera skeleton

  // Get relevant camera vectors
  Vector3 root, lookDir, upDir, rightDir;
  std::array<Vector3, 4> framePoints;
  std::array<Vector3, 3> dirFrame;
  getCameraPoints(root, framePoints, dirFrame);

  for(int i = 0; i < 4; i++) {
    addTriangle(root, framePoints[i], framePoints[(i+1)%4]);
  }

  // Store data in buffers
  pickProgram->setAttribute("a_position", positions);
  pickProgram->setAttribute("a_color", pickColors);
}

void CameraView::prepareCameraSkeleton() {
  // Create the GL program
  cameraSkeletonProgram = new gl::GLProgram(&WIREFRAME_VERT_SHADER, &WIREFRAME_FRAG_SHADER, gl::DrawMode::Lines);

  // Relevant points in world space
  cameraSkeletonScale = state::lengthScale;
  Vector3 root, lookDir, upDir, rightDir;
  std::array<Vector3, 4> framePoints;
  std::array<Vector3, 3> dirFrame;
  getCameraPoints(root, framePoints, dirFrame);
  lookDir = dirFrame[0];
  upDir = dirFrame[1];
  rightDir = dirFrame[2];

  // Triangles to draw
  std::vector<Vector3> positions; // position in space

  // Add lines
  for (int i = 0; i < 4; i++) {
    // From root to corner of frame
    positions.push_back(root);
    positions.push_back(framePoints[i]);

    // Around frame
    positions.push_back(framePoints[i]);
    positions.push_back(framePoints[(i + 1) % 4]);
  }

  // Store data in buffers
  cameraSkeletonProgram->setAttribute("a_position", positions);
}

void CameraView::getCameraPoints(Vector3& rootV, std::array<Vector3, 4>& framePoints,
                                 std::array<Vector3, 3>& dirFrame) {
  glm::vec3 root = parameters.getPosition();
  glm::vec3 lookDir = parameters.getLookDir();
  glm::vec3 upDir = parameters.getUpDir();
  glm::vec3 rightDir = parameters.getRightDir();

  float cameraDrawSize = cameraSkeletonScale * 0.01;
  float frameDrawWidth = std::tan(glm::radians(parameters.fov / 2.f)) * cameraDrawSize;
  float frameDrawHeight = std::tan(glm::radians(parameters.fov / 2.f)) * cameraDrawSize;

  glm::vec3 upperLeft = root + cameraDrawSize * lookDir + upDir * frameDrawHeight - rightDir * frameDrawWidth;
  glm::vec3 lowerLeft = root + cameraDrawSize * lookDir - upDir * frameDrawHeight - rightDir * frameDrawWidth;
  glm::vec3 upperRight = root + cameraDrawSize * lookDir + upDir * frameDrawHeight + rightDir * frameDrawWidth;
  glm::vec3 lowerRight = root + cameraDrawSize * lookDir - upDir * frameDrawHeight + rightDir * frameDrawWidth;

  rootV = toV(root);
  framePoints[0] = toV(upperRight);
  framePoints[1] = toV(upperLeft);
  framePoints[2] = toV(lowerLeft);
  framePoints[3] = toV(lowerRight);

  dirFrame[0] = toV(lookDir);
  dirFrame[1] = toV(upDir);
  dirFrame[2] = toV(rightDir);
}

void CameraView::drawSharedStructureUI() { ImGui::SliderFloat("Opaque", &globalImageTransparency, 0.0, 1.0, "%.2f"); }

void CameraView::drawPickUI(size_t localPickID) {

  // Fly to on a double click
  if (pick::pickWasDoubleClick) {
    view::startFlightTo(parameters, .3);
    pick::pickWasDoubleClick = false; // consume the double click
  }

  ImGui::TextUnformatted(("Camera " + name).c_str());
}

void CameraView::drawUI() {
  if (ImGui::TreeNode(name.c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();

    if (ImGui::Button("Fly to")) {
      view::startFlightTo(parameters, .3);
    }

    { // Pick an image to show

      // Find the current image
      int currImage = 0; // none
      int i = 1;
      std::vector<const char*> nameStrings;
      const char* noneStr = "None";
      nameStrings.push_back(noneStr);
      for (auto& kv : images) {
        nameStrings.push_back(kv.first.c_str());
        if (kv.second == activeImage) {
          currImage = i;
        }
        i++;
      }

      // Draw the GUI element
      int newInd = currImage;
      ImGui::ListBox("", &newInd, &nameStrings[0], nameStrings.size());

      // Update the image if requested
      if (newInd != currImage) {
        if (newInd == 0) {
          clearActiveImage();
        } else {
          setActiveImage(nameStrings[newInd]);
        }
      }
    }

    ImGui::TreePop();
  }
}

void CameraView::addImage(std::string name, unsigned char* I, size_t width, size_t height) {
  if (images.find(name) != images.end()) {
    // error("Image name " + name + " is already in use");
    // return;
    delete images[name];
    images.erase(name);
  }

  Image* i = new Image(name, I, width, height);
  images[name] = i;

  // Make first image active
  if (images.size() == 1) {
    setActiveImage(name);
  }
}

void CameraView::removeImage(std::string name, bool errorIfNotPresent) {

  if (images.find(name) == images.end()) {
    if (errorIfNotPresent) {
      error("Tried to remove image which does not exist.");
    }
    return;
  }

  Image* im = images[name];

  if (im == activeImage) {
    clearActiveImage();
  }

  images.erase(name);
  delete im;
}

void CameraView::setActiveImage(std::string name) {
  if (images.find(name) == images.end()) {
    error("No image with name " + name);
    return;
  }
  Image* im = images[name];
  activeImage = im;

  // == Create the program to draw the image

  // Delete the old program, if present
  if (imageViewProgram != nullptr) {
    delete imageViewProgram;
  }

  // Create the new program
  imageViewProgram =
      new gl::GLProgram(&PROJECTEDIMAGE_VERT_SHADER, &PROJECTEDIMAGE_FRAG_SHADER, gl::DrawMode::Triangles);

  // Push the texture to the buffer
  imageViewProgram->setTexture2D("t_image", im->data, im->width, im->height, true);

  // The frame on which we will draw

  // Get properties of the frame
  Vector3 root;
  std::array<Vector3, 4> framePoints;
  std::array<Vector3, 3> dirFrame;
  getCameraPoints(root, framePoints, dirFrame);
  std::array<Vector2, 4> frameCoords = {{Vector2{1, 0}, Vector2{0, 0}, Vector2{0, 1}, Vector2{1, 1}}};

  // The two triangles which compose the frame
  std::vector<std::array<int, 3>> tris = {{{0, 1, 3}}, {{1, 2, 3}}};

  // Build position and texture coord buffers
  std::vector<Vector3> positions;
  std::vector<Vector2> tCoords;
  for (auto t : tris) {
    for (int ind : t) {
      positions.push_back(framePoints[ind]);
      tCoords.push_back(frameCoords[ind]);
    }
  }

  imageViewProgram->setAttribute("a_position", positions);
  imageViewProgram->setAttribute("a_tCoord", tCoords);
}

void CameraView::clearActiveImage() {
  if (imageViewProgram != nullptr) {
    delete imageViewProgram;
    imageViewProgram = nullptr;
  }
  activeImage = nullptr;
}

double CameraView::lengthScale() { return 0; }

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> CameraView::boundingBox() {
  Vector3 pos = location();
  return std::make_tuple(pos, pos);
}

Image::Image(std::string name_, unsigned char* data_, size_t width_, size_t height_)
    : name(name_), width(width_), height(height_) {
  size_t aSize = width * height * 3;
  data = new unsigned char[width * height * 3];
  std::copy(data_, data_ + aSize, data);
}

Image::~Image() { delete[] data; }

} // namespace polyscope

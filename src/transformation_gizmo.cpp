#include "polyscope/transformation_gizmo.h"

#include "polyscope/polyscope.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <cmath>

namespace polyscope {

TransformationGizmo::TransformationGizmo(std::string name_, glm::mat4& T_, PersistentValue<glm::mat4>* Tpers_)
    : name(name_), enabled(name + "#name", false), T(T_), Tpers(Tpers_)

{}

void TransformationGizmo::markUpdated() {
  if (Tpers != nullptr) {
    Tpers->manuallyChanged();
  }
}

void TransformationGizmo::prepare() {

  { // The rotation rings, drawn via textured quads
    ringProgram =
        render::engine->requestShader("TRANSFORMATION_GIZMO_ROT", {}, render::ShaderReplacementDefaults::Process);

    std::vector<glm::vec3> coords;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> components;
    std::tie(coords, normals, colors, texcoords, components) = triplePlaneCoords();

    ringProgram->setAttribute("a_position", coords);
    ringProgram->setAttribute("a_normal", normals);
    ringProgram->setAttribute("a_color", colors);
    ringProgram->setAttribute("a_texcoord", texcoords);
    ringProgram->setAttribute("a_component", components);

    // render::engine->setMaterial(*ringProgram, "wax");
  }

  { // Translation arrows
    arrowProgram = render::engine->requestShader(
        "RAYCAST_VECTOR", {"VECTOR_PROPAGATE_COLOR", "TRANSFORMATION_GIZMO_VEC", "SHADE_COLOR", "LIGHT_MATCAP"},
        render::ShaderReplacementDefaults::Process);

    std::vector<glm::vec3> vectors;
    std::vector<glm::vec3> bases;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> components;
    std::tie(vectors, bases, colors, components) = tripleArrowCoords();

    arrowProgram->setAttribute("a_vector", vectors);
    arrowProgram->setAttribute("a_position", bases);
    arrowProgram->setAttribute("a_color", colors);
    arrowProgram->setAttribute("a_component", components);

    render::engine->setMaterial(*arrowProgram, "wax");
  }

  { // Scale sphere
    sphereProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SHADE_BASECOLOR", "LIGHT_MATCAP"},
                                                  render::ShaderReplacementDefaults::Process);
    render::engine->setMaterial(*sphereProgram, "wax");

    std::vector<glm::vec3> center = {glm::vec3(0., 0., 0.)};
    sphereProgram->setAttribute("a_position", center);
  }
}

void TransformationGizmo::draw() {
  if (!enabled.get()) return;
  if (!ringProgram) prepare();

  // == set uniforms

  float transScale = glm::length(glm::vec3(T[0]));
  float gizmoSizePreTrans = gizmoSizeRel * state::lengthScale;
  float gizmoSize = transScale * gizmoSizePreTrans;

  glm::mat4 viewMat =
      view::getCameraViewMatrix() * T * glm::scale(glm::vec3{gizmoSizePreTrans, gizmoSizePreTrans, gizmoSizePreTrans});
  ringProgram->setUniform("u_modelView", glm::value_ptr(viewMat));
  arrowProgram->setUniform("u_modelView", glm::value_ptr(viewMat));
  sphereProgram->setUniform("u_modelView", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  ringProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));
  arrowProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));
  sphereProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

  ringProgram->setUniform("u_diskWidthRel", diskWidthObj);

  // set selections
  glm::vec3 selectRot{0., 0., 0.};
  glm::vec3 selectTrans{0., 0., 0.};
  glm::vec3 sphereColor(0.85);
  if (selectedType == TransformHandle::Rotation) {
    selectRot[selectedDim] = 1.;
  }
  ringProgram->setUniform("u_active", selectRot);
  if (selectedType == TransformHandle::Translation) {
    selectTrans[selectedDim] = 1.;
  }
  arrowProgram->setUniform("u_active", selectTrans);
  if (selectedType == TransformHandle::Scale) {
    sphereColor = glm::vec3(0.95);
  }

  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  arrowProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  arrowProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  arrowProgram->setUniform("u_lengthMult", vecLength);
  arrowProgram->setUniform("u_radius", 0.2 * gizmoSize);

  sphereProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  sphereProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

  sphereProgram->setUniform("u_pointRadius", sphereRad * gizmoSize);
  sphereProgram->setUniform("u_baseColor", sphereColor);

  render::engine->setDepthMode(DepthMode::Less);
  render::engine->setBlendMode();
  render::engine->setBackfaceCull(false);

  // == draw

  ringProgram->draw();
  arrowProgram->draw();
  sphereProgram->draw();
}


bool TransformationGizmo::interact() {
  if (!enabled.get()) return false;

  ImGuiIO& io = ImGui::GetIO();

  // end a drag, if needed
  bool draggingAtStart = currentlyDragging;
  if (currentlyDragging && (!ImGui::IsMouseDown(0) || !ImGui::IsMousePosValid())) {
    currentlyDragging = false;
  }

  // Get the mouse ray in world space
  glm::vec3 raySource = view::getCameraWorldPosition();
  glm::vec2 mouseCoords{io.MousePos.x, io.MousePos.y};
  glm::vec3 ray = view::screenCoordsToWorldRay(mouseCoords);

  // Get data about the widget
  // (there are much more efficient ways to do this)
  glm::vec3 center(T * glm::vec4{0., 0., 0., 1.});
  glm::vec3 nX(T * glm::vec4{1., 0., 0., 0.});
  glm::vec3 nY(T * glm::vec4{0., 1., 0., 0.});
  glm::vec3 nZ(T * glm::vec4{0., 0., 1., 0.});
  std::array<glm::vec3, 3> axNormals{glm::normalize(nX), glm::normalize(nY), glm::normalize(nZ)};

  float transScale = glm::length(glm::vec3(T[0]));
  float gizmoSize = transScale * gizmoSizeRel * state::lengthScale;
  float diskRad = gizmoSize; // size 1
  float diskWidth = gizmoSize * diskWidthObj;

  if (currentlyDragging) {

    if (selectedType == TransformHandle::Rotation) {

      // Cast against the axis we are currently rotating
      glm::vec3 normal = axNormals[selectedDim];
      float dist, tHit;
      glm::vec3 nearestPoint;
      std::tie(tHit, dist, nearestPoint) = circleTest(raySource, ray, center, normal, diskRad - diskWidth);

      if (dist != std::numeric_limits<float>::infinity()) {
        // if a collinear line (etc) causes an inf dist, just don't process

        // Compute the new nearest normal to the drag
        glm::vec3 nearestDir = glm::normalize(nearestPoint - center);
        float arg = glm::dot(normal, glm::cross(dragPrevVec, nearestDir));
        arg = glm::clamp(arg, -1.f, 1.f);
        float angle = std::asin(arg); // could be fancier with atan, but that's fine

        // Split, transform, and recombine
        glm::vec4 trans(T[3]);
        T[3] = glm::vec4{0., 0., 0., 1.};
        // T.get() = glm::translate(glm::rotate(angle, normal) * glm::translate(T.get(), -trans), trans);
        T = glm::rotate(angle, normal) * T;
        T[3] = trans;
        markUpdated();
        polyscope::requestRedraw();

        dragPrevVec = nearestDir; // store this dir for the next time around
      }
    } else if (selectedType == TransformHandle::Translation) {

      // Cast against the axis we are currently translating

      glm::vec3 normal = axNormals[selectedDim];
      float dist, tHit;
      glm::vec3 nearestPoint;
      std::tie(tHit, dist, nearestPoint) =
          lineTest(raySource, ray, center, normal, std::numeric_limits<float>::infinity());

      if (dist != std::numeric_limits<float>::infinity()) {
        // if a collinear line (etc) causes an inf dist, just don't process

        // Split, transform, and recombine
        glm::vec3 trans = nearestPoint - dragPrevVec;
        T[3][0] += trans.x;
        T[3][1] += trans.y;
        T[3][2] += trans.z;
        markUpdated();
        polyscope::requestRedraw();

        dragPrevVec = nearestPoint; // store this dir for the next time around
      }

    } else if (selectedType == TransformHandle::Scale) {

      // Cast against the scale sphere

      float dist, tHit;
      glm::vec3 nearestPoint;
      float worldSphereRad = sphereRad * gizmoSize;
      std::tie(tHit, dist, nearestPoint) = sphereTest(raySource, ray, center, worldSphereRad, false);

      if (dist != std::numeric_limits<float>::infinity()) {
        // if a collinear line (etc) causes an inf dist, just don't process

        float newWorldRad = glm::length(nearestPoint - center);
        float scaleRatio = newWorldRad / worldSphereRad;

        // Split, transform, and recombine
        glm::vec4 trans(T[3]);
        T[3] = glm::vec4{0., 0., 0., 1.};
        T *= scaleRatio;
        T[3] = trans;
        markUpdated();
        polyscope::requestRedraw();

        dragPrevVec = nearestPoint; // store this dir for the next time around
      }
    }
  } else /* !currentlyDragging */ {

    // == Find the part of the widget that we are closest to

    float firstHit = std::numeric_limits<float>::infinity();
    float hitDist = std::numeric_limits<float>::infinity();
    int hitDim = -1;
    TransformHandle hitType = TransformHandle::None;
    glm::vec3 hitNearest(0., 0., 0.);

    // Test the three rotation directions
    for (int dim = 0; dim < 3; dim++) {

      glm::vec3 normal = axNormals[dim];

      float dist, tHit;
      glm::vec3 nearestPoint;
      std::tie(tHit, dist, nearestPoint) = circleTest(raySource, ray, center, normal, diskRad - diskWidth);

      if (dist < diskWidth && tHit < firstHit) {
        firstHit = tHit;
        hitDist = dist;
        hitDim = dim;
        hitType = TransformHandle::Rotation;
        hitNearest = nearestPoint;
      }
    }

    // Test the three translation directions
    for (int dim = 0; dim < 3; dim++) {

      glm::vec3 normal = axNormals[dim];

      float dist, tHit;
      glm::vec3 nearestPoint;
      std::tie(tHit, dist, nearestPoint) = lineTest(raySource, ray, center, normal, vecLength * gizmoSize);
      tHit -= diskWidth; // pull the hit outward, hackily simulates cylinder radius

      if (dist < diskWidth && tHit < firstHit) {
        firstHit = tHit;
        hitDist = dist;
        hitDim = dim;
        hitType = TransformHandle::Translation;
        hitNearest = nearestPoint;
      }
    }

    { // Test the scaling sphere

      float dist, tHit;
      glm::vec3 nearestPoint;
      std::tie(tHit, dist, nearestPoint) = sphereTest(raySource, ray, center, sphereRad * gizmoSize);

      if (dist == 0. && tHit < firstHit) {
        firstHit = tHit;
        hitDist = dist;
        hitDim = 0;
        hitType = TransformHandle::Scale;
        hitNearest = nearestPoint;
      }
    }

    // = Process the result

    // clear selection before proceeding
    selectedType = TransformHandle::None;
    selectedDim = -1;
    bool dragStarted = false;

    if (hitType == TransformHandle::Rotation && hitDist < diskWidth) {
      // rotation is hovered

      // set new selection
      selectedType = TransformHandle::Rotation;
      selectedDim = hitDim;

      // if the mouse is clicked, start a drag
      if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
        currentlyDragging = true;
        dragStarted = true;

        glm::vec3 nearestDir = glm::normalize(hitNearest - center);
        dragPrevVec = nearestDir;
      }
    } else if (hitType == TransformHandle::Translation && hitDist < diskWidth) {
      // translation is hovered

      // set new selection
      selectedType = TransformHandle::Translation;
      selectedDim = hitDim;

      // if the mouse is clicked, start a drag
      if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
        currentlyDragging = true;
        dragStarted = true;

        dragPrevVec = hitNearest;
      }
    } else if (hitType == TransformHandle::Scale) {
      // scaling is hovered

      // set new selection
      selectedType = TransformHandle::Scale;
      selectedDim = -1;

      // if the mouse is clicked, start a drag
      if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
        currentlyDragging = true;
        dragStarted = true;

        dragPrevVec = hitNearest;
      }
    }
  }

  return currentlyDragging || draggingAtStart;
} // namespace polyscope

std::tuple<float, float, glm::vec3> TransformationGizmo::circleTest(glm::vec3 raySource, glm::vec3 rayDir,
                                                                    glm::vec3 center, glm::vec3 normal, float radius) {

  // used for explicit constructors below to make old compilers (gcc-5) happy
  typedef std::tuple<float, float, glm::vec3> ret_t;

  // Intersect the ray with the plane defined by the normal
  float div = glm::dot(normal, rayDir);
  if (std::fabs(div) < 1e-6)
    return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}}; // parallel

  float tRay = glm::dot((center - raySource), normal) / div;
  if (tRay < 0) return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}}; // behind the ray

  glm::vec3 hitPoint = raySource + tRay * rayDir;

  // Find the closest point on the circle
  float hitRad = glm::length(hitPoint - center);
  float ringDist = std::fabs(hitRad - radius);
  glm::vec3 nearestPoint = center + (hitPoint - center) / hitRad * radius;

  return ret_t{tRay, ringDist, nearestPoint};
}

std::tuple<float, float, glm::vec3> TransformationGizmo::lineTest(glm::vec3 raySource, glm::vec3 rayDir,
                                                                  glm::vec3 center, glm::vec3 tangent, float length) {

  // used for explicit constructors below to make old compilers (gcc-5) happy
  typedef std::tuple<float, float, glm::vec3> ret_t;

  glm::vec3 nBetween = glm::cross(rayDir, tangent);
  if (glm::length(nBetween) < 1e-6)
    return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}}; // parallel

  glm::vec3 nTan = glm::cross(tangent, nBetween);
  glm::vec3 nRay = glm::cross(rayDir, nBetween);

  float tRay = glm::dot(center - raySource, nTan) / glm::dot(rayDir, nTan);
  float tLine = glm::dot(raySource - center, nRay) / glm::dot(tangent, nRay);

  if (tLine < -length || tLine > length || tRay < 0)
    return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}}; // out of bounds or beind

  glm::vec3 pRay = raySource + tRay * rayDir;
  glm::vec3 pLine = center + tLine * tangent;

  return ret_t{tRay, glm::length(pRay - pLine), pLine};
}

std::tuple<float, float, glm::vec3> TransformationGizmo::sphereTest(glm::vec3 raySource, glm::vec3 rayDir,
                                                                    glm::vec3 center, float radius,
                                                                    bool allowHitSurface) {

  // used for explicit constructors below to make old compilers (gcc-5) happy
  typedef std::tuple<float, float, glm::vec3> ret_t;

  glm::vec3 oc = raySource - center;
  float b = 2. * dot(oc, rayDir);
  float c = glm::dot(oc, oc) - radius * radius;
  float disc = b * b - 4 * c;
  if (disc < 1e-6 || !allowHitSurface) {
    // miss, return nearest point
    float tHit = glm::dot(rayDir, center - raySource);
    if (tHit < 0.) {
      // hit behind
      return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}};
    }
    glm::vec3 hitPoint = raySource + tHit * rayDir;
    return ret_t{tHit, glm::length(hitPoint - center) - radius, hitPoint};
  } else {
    // actual hit
    float tHit = (-b - std::sqrt(disc)) / 2.;
    if (tHit < 0.) {
      // hit behind
      return ret_t{-1, std::numeric_limits<float>::infinity(), glm::vec3{0., 0., 0.}};
    }
    return ret_t{tHit, 0, raySource + tHit * rayDir};
  }
}

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec2>,
           std::vector<glm::vec3>>
TransformationGizmo::triplePlaneCoords() {
  std::vector<glm::vec3> coords;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> colors;
  std::vector<glm::vec2> texcoords;
  std::vector<glm::vec3> components;

  float s = 1.2;

  auto addPlane = [&](int i) {
    int j = (i + 1) % 3;
    int k = (i + 2) % 3;

    glm::vec3 lowerLeft(0., 0., 0.);
    lowerLeft[j] = -s;
    lowerLeft[k] = -s;
    glm::vec2 lowerLeftT(-s, -s);

    glm::vec3 lowerRight(0., 0., 0.);
    lowerRight[j] = s;
    lowerRight[k] = -s;
    glm::vec2 lowerRightT(s, -s);

    glm::vec3 upperLeft(0., 0., 0.);
    upperLeft[j] = -s;
    upperLeft[k] = s;
    glm::vec2 upperLeftT(-s, s);

    glm::vec3 upperRight(0., 0., 0.);
    upperRight[j] = s;
    upperRight[k] = s;
    glm::vec2 upperRightT(s, s);

    glm::vec3 normal(0., 0., 0.);
    normal[i] = 1;
    glm::vec3 component(0., 0., 0.);
    component[i] = 1;

    // First triangle
    coords.push_back(lowerLeft);
    coords.push_back(lowerRight);
    coords.push_back(upperRight);
    for (int m = 0; m < 3; m++) {
      normals.push_back(normal);
      components.push_back(component);
      colors.push_back(niceRGB[i]);
    }
    texcoords.push_back(lowerLeftT);
    texcoords.push_back(lowerRightT);
    texcoords.push_back(upperRightT);

    // Second triangle
    coords.push_back(lowerLeft);
    coords.push_back(upperRight);
    coords.push_back(upperLeft);
    for (int m = 0; m < 3; m++) {
      normals.push_back(normal);
      components.push_back(component);
      colors.push_back(niceRGB[i]);
    }
    texcoords.push_back(lowerLeftT);
    texcoords.push_back(upperRightT);
    texcoords.push_back(upperLeftT);
  };

  // apparently this is how we write for loops in 2021...
  addPlane(0);
  addPlane(1);
  addPlane(2);

  return std::make_tuple(coords, normals, colors, texcoords, components);
}

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>>
TransformationGizmo::tripleArrowCoords() {

  std::vector<glm::vec3> vectors;
  std::vector<glm::vec3> bases;
  std::vector<glm::vec3> color;
  std::vector<glm::vec3> components;

  for (int dim = 0; dim < 3; dim++) {

    // Forward vec
    bases.push_back(glm::vec3{0., 0., 0.});
    glm::vec3 v(0., 0., 0.);
    v[dim] = 1;
    vectors.push_back(v);
    color.push_back(niceRGB[dim]);
    components.push_back(v);

    // Backward vec
    bases.push_back(glm::vec3{0., 0., 0.});
    v[dim] = -1;
    vectors.push_back(v);
    v[dim] = 1;
    color.push_back(niceRGB[dim]);
    components.push_back(v);
  }

  return std::make_tuple(vectors, bases, color, components);
}

/*
std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>> TransformationGizmo::unitCubeCoords() {
  std::vector<glm::vec3> coords;
  std::vector<glm::vec3> normals;

  auto addCubeFace = [&](int iS, float s) {
    int iU = (iS + 1) % 3;
    int iR = (iS + 2) % 3;

    glm::vec3 lowerLeft(0., 0., 0.);
    lowerLeft[iS] = s;
    lowerLeft[iU] = -s;
    lowerLeft[iR] = -s;

    glm::vec3 lowerRight(0., 0., 0.);
    lowerRight[iS] = s;
    lowerRight[iU] = -s;
    lowerRight[iR] = s;

    glm::vec3 upperLeft(0., 0., 0.);
    upperLeft[iS] = s;
    upperLeft[iU] = s;
    upperLeft[iR] = -s;

    glm::vec3 upperRight(0., 0., 0.);
    upperRight[iS] = s;
    upperRight[iU] = s;
    upperRight[iR] = s;

    glm::vec3 normal(0., 0., 0.);
    normal[iS] = s;

    // first triangle
    coords.push_back(lowerLeft);
    coords.push_back(lowerRight);
    coords.push_back(upperRight);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);

    // second triangle
    coords.push_back(lowerLeft);
    coords.push_back(upperRight);
    coords.push_back(upperLeft);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
  };

  addCubeFace(0, +1.);
  addCubeFace(0, -1.);
  addCubeFace(1, +1.);
  addCubeFace(1, -1.);
  addCubeFace(2, +1.);
  addCubeFace(2, -1.);

  return {coords, normals};
}
*/
} // namespace polyscope

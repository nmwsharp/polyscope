#pragma once

namespace polyscope {

template <class T1, class T2, class T3>
CameraExtrinsics CameraExtrinsics::fromVectors(const T1& root, const T2& lookDir, const T3& upDir) {

  glm::vec3 rootGLM = standardizeVector3D<glm::vec3, T1>(root);
  glm::vec3 lookDirGLM = standardizeVector3D<glm::vec3, T1>(lookDir);
  glm::vec3 upDirGLM = standardizeVector3D<glm::vec3, T1>(upDir);

  glm::mat4 E = glm::lookAt(rootGLM, rootGLM + glm::normalize(lookDirGLM), glm::normalize(upDirGLM));
  return CameraExtrinsics(E);
}
} // namespace polyscope

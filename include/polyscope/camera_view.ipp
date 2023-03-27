// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


namespace polyscope {

template <class T1, class T2, class T3>
CameraView* registerCameraView(std::string name, const T1& root, const T2& lookDir, const T3& upDir, float fovVertDeg,
                               float aspectRatio) {

  glm::vec3 rootGLM = standardizeVector3D<glm::vec3, T1>(root);
  glm::vec3 lookDirGLM = standardizeVector3D<glm::vec3, T1>(lookDir);
  glm::vec3 upDirGLM = standardizeVector3D<glm::vec3, T1>(upDir);

  CameraParameters params(rootGLM, lookDirGLM, upDirGLM, fovVertDeg, aspectRatio);

  CameraView* s = new CameraView(name, params);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

inline CameraView* registerCameraView(std::string name, CameraParameters params) {
  CameraView* s = new CameraView(name, params);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

template <class T1, class T2, class T3>
void CameraView::updateCameraParameters(const T1& root, const T2& lookDir, const T3& upDir, float fovVertDeg,
                                        float aspectRatio) {

  glm::vec3 rootGLM = standardizeVector3D<glm::vec3, T1>(root);
  glm::vec3 lookDirGLM = standardizeVector3D<glm::vec3, T1>(lookDir);
  glm::vec3 upDirGLM = standardizeVector3D<glm::vec3, T1>(upDir);

  CameraParameters params(rootGLM, lookDirGLM, upDirGLM, fovVertDeg, aspectRatio);

  updateCameraParameters(params);
}

} // namespace polyscope

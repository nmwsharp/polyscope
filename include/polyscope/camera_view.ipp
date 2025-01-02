// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


namespace polyscope {

inline CameraView* registerCameraView(std::string name, CameraParameters params) {
  CameraView* s = new CameraView(name, params);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

// Shorthand to get a camera view from polyscope
inline CameraView* getCameraView(std::string name) {
  return dynamic_cast<CameraView*>(getStructure(CameraView::structureTypeName, name));
}
inline bool hasCameraView(std::string name) { return hasStructure(CameraView::structureTypeName, name); }
inline void removeCameraView(std::string name, bool errorIfAbsent) {
  removeStructure(CameraView::structureTypeName, name, errorIfAbsent);
}


} // namespace polyscope

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================


class SurfaceParameterizationQuantity : public SurfaceMeshQuantity {

public:
  SurfaceParameterizationQuantity(std::string name, ParamCoordsType type_, SurfaceMesh& mesh_);

  void draw() override;
  virtual void buildCustomUI() override;

  virtual void geometryChanged() override;


  // === Members
  ParamCoordsType coordsType;

  // The program which does the drawing
  std::unique_ptr<gl::GLProgram> program;

  // === Viz stuff
  // to keep things simple, has settings for all of the viz styles, even though not all are used at all times

  SurfaceParameterizationQuantity* setStyle(ParamVizStyle newStyle);

  float modLen = .05; // for all, period of the checker / stripes

  glm::vec3 checkColor1, checkColor2; // for checker (two colors to use)

  glm::vec3 gridLineColor, gridBackgroundColor; // for GRID (two colors to use)

  int iColorMap = 0;   // for LOCAL (color map index)
  float localRot = 0.; // for LOCAL (angular shift, in radians)

protected:
  ParamVizStyle vizStyle = ParamVizStyle::CHECKER;

  // Helpers
  void createProgram();
  void setProgramUniforms(gl::GLProgram& program);
  virtual void fillColorBuffers(gl::GLProgram& p) = 0;
};


// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================

class SurfaceCornerParameterizationQuantity : public SurfaceParameterizationQuantity {

public:
  SurfaceCornerParameterizationQuantity(std::string name, std::vector<glm::vec2> values_, ParamCoordsType type_,
                                        SurfaceMesh& mesh_);

  virtual void buildHalfedgeInfoGUI(size_t heInd) override;
  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec2> coords; // on corners

protected:
  virtual void fillColorBuffers(gl::GLProgram& p) override;
};


// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================

class SurfaceVertexParameterizationQuantity : public SurfaceParameterizationQuantity {
public:
  SurfaceVertexParameterizationQuantity(std::string name, std::vector<glm::vec2> values_, ParamCoordsType type_,
                                        SurfaceMesh& mesh_);

  virtual void buildVertexInfoGUI(size_t vInd) override;
  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec2> coords; // on vertices

protected:
  virtual void fillColorBuffers(gl::GLProgram& p) override;
};


// ==============================================================
// ===============  Parameterization Adders  ====================
// ==============================================================

// Standard a parameterization, defined at corners
template <class T>
void SurfaceMesh::addParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type) {
  validateSize(coords, cornerDataSize, "parameterization quantity " + name);

  SurfaceCornerParameterizationQuantity* q = new SurfaceCornerParameterizationQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec2, T, 2>(coords), cornerPerm), type, *this);
  addQuantity(q);
}

// Parameterization defined at vertices, rather than corners
template <class T>
void SurfaceMesh::addVertexParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);

  SurfaceVertexParameterizationQuantity* q = new SurfaceVertexParameterizationQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec2, T, 2>(coords), vertexPerm), type, *this);
  addQuantity(q);
}

// "local" parameterization defined at vertices. has different presets: type is WORLD and style is LOCAL
template <class T>
void SurfaceMesh::addLocalParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type) {
  validateSize(coords, vertexDataSize, "parameterization (at vertices) quantity " + name);

  SurfaceVertexParameterizationQuantity* q = new SurfaceVertexParameterizationQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec2, T, 2>(coords), vertexPerm), type, *this);
  addQuantity(q);

  q->setStyle(ParamVizStyle::LOCAL_CHECK);
}


} // namespace polyscope


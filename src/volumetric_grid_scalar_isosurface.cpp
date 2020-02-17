#include "polyscope/volumetric_grid_scalar_isosurface.h"
#include "polyscope/marchingcubes/mesh_implicit_surface.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"

namespace polyscope {

VolumetricGridScalarIsosurface::VolumetricGridScalarIsosurface(std::string name, VolumetricGrid& grid_,
                                                               const std::vector<double>& values_, double levelSet_)
    : VolumetricGridQuantity(name, grid_, true), values(std::move(values_)) {
  levelSet = levelSet_;
  meshIsosurface();
  increment = 0.01;
  newLevelSet = levelSet;
}

void VolumetricGridScalarIsosurface::recomputeNormals() {
  normals.clear();
  normals.resize(nodes.size());

  for (size_t i = 0; i < normals.size(); i++) {
    normals[i] = glm::vec3{0., 0., 0.};
  }

  for (auto& tri : triangles) {
    glm::vec3 v0 = nodes[tri[0]];
    glm::vec3 v1 = nodes[tri[1]];
    glm::vec3 v2 = nodes[tri[2]];

    glm::vec3 e01 = v1 - v0;
    glm::vec3 e02 = v2 - v0;
    glm::vec3 areaWtNormal = glm::cross(e01, e02);

    normals[tri[0]] += areaWtNormal;
    normals[tri[1]] += areaWtNormal;
    normals[tri[2]] += areaWtNormal;
  }

  for (size_t i = 0; i < normals.size(); i++) {
    normals[i] = glm::normalize(normals[i]);
  }
}

void VolumetricGridScalarIsosurface::meshIsosurface() {
  nodes.clear();
  triangles.clear();

  size_t nCornersPerSide = parent.nCornersPerSide;
  glm::vec3 gridCenter = parent.gridCenter;
  double sideLength = parent.sideLength;
  marchingcubes::MeshImplicitGrid(values, levelSet, nCornersPerSide, gridCenter, sideLength, nodes, triangles);

  std::cout << "Meshed isosurface, producing " << nodes.size() << " nodes, " << triangles.size() << " triangles"
            << std::endl;
  recomputeNormals();

  prepare();
}

void VolumetricGridScalarIsosurface::draw() {
  if (!isEnabled()) return;

  if (meshProgram == nullptr) {
    prepare();
  }
  
  // Set uniforms
  parent.setTransformUniforms(*meshProgram);
  setProgramUniforms(*meshProgram);
  meshProgram->draw();
}

void VolumetricGridScalarIsosurface::buildCustomUI() {
  ImGui::Text("Current isolevel: %.4f", levelSet);
  ImGui::PushItemWidth(100);
  ImGui::InputDouble(" ", &newLevelSet);

  ImGui::SameLine();

  if (ImGui::Button("Set isolevel")) {
    levelSet = newLevelSet;
    meshIsosurface();
  }

  ImGui::Spacing();

  ImGui::Text("Increment level:");
  ImGui::SameLine();
  if (ImGui::Button("-", ImVec2{32, 20})) {
    levelSet -= increment;
    newLevelSet = levelSet;
    meshIsosurface();
  }
  ImGui::SameLine();
  if (ImGui::Button("+", ImVec2{32, 20})) {
    levelSet += increment;
    newLevelSet = levelSet;
    meshIsosurface();
  }

  ImGui::InputDouble("Step size", &increment);
  ImGui::PopItemWidth();
}

void VolumetricGridScalarIsosurface::setProgramUniforms(gl::GLProgram& program) {
  meshProgram->setUniform("u_basecolor", parent.getColor());
}

std::string VolumetricGridScalarIsosurface::niceName() { return name + " (isosurface)"; }

void VolumetricGridScalarIsosurface::geometryChanged() {
  // For now, nothing
}

void VolumetricGridScalarIsosurface::prepareTriangleIndices() {
  // Need to upload triangle indices
  // Indicies are supplied as size_t but need to be cast to unsigned int
  std::vector<std::array<unsigned int, 3>> index_triangles(triangles.size());
  for (size_t i = 0; i < index_triangles.size(); i++) {
    size_t ui_max = (size_t)INT_MAX * 2;
    if (triangles[i][0] > ui_max || triangles[i][1] > ui_max || triangles[i][2] > ui_max) {
      std::cerr << "Number of triangles exceeds unsigned int max; exiting" << std::endl;
      exit(1);
    }
    index_triangles[i] = {static_cast<unsigned int>(triangles[i][0]), static_cast<unsigned int>(triangles[i][1]),
                          static_cast<unsigned int>(triangles[i][2])};
  }

  meshProgram->setIndex(index_triangles);
}

void VolumetricGridScalarIsosurface::prepare() {
  // TODO: figure out what shaders to use
  meshProgram.reset(new gl::GLProgram(&gl::PLAIN_SURFACE_VERT_SHADER, &gl::PLAIN_SURFACE_FRAG_SHADER,
                                      gl::DrawMode::IndexedTriangles));

  prepareTriangleIndices();

  // Populate draw buffers
  fillGeometryBuffersSmooth(*meshProgram);

  setMaterialForProgram(*meshProgram, "wax");
}


void VolumetricGridScalarIsosurface::fillGeometryBuffersSmooth(gl::GLProgram& p) {
  std::vector<glm::vec3> bcoord;
  size_t nFaces = triangles.size();
  bool wantsBary = p.hasAttribute("a_barycoord");

  if (wantsBary) {
    bcoord.reserve(3 * nFaces);
  }

  for (size_t iF = 0; iF < nFaces; iF++) {
    if (wantsBary) {
      bcoord.push_back(glm::vec3{1., 0., 0.});
      bcoord.push_back(glm::vec3{0., 1., 0.});
      bcoord.push_back(glm::vec3{0., 0., 1.});
    }
  }

  // Store data in buffers
  p.setAttribute("a_position", nodes);
  p.setAttribute("a_normal", normals);
  if (wantsBary) {
    p.setAttribute("a_barycoord", bcoord);
  }
}

} // namespace polyscope

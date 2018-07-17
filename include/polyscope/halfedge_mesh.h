#pragma once

#include <array>
#include <vector>

#include <polyscope/utilities.h>

namespace polyscope {


// A quick and dirty halfedge mesh datastructure, with some additional functionality useful for visualization tasks in
// Polyscope. Properly speaking, a halfedge mesh should track only the _connectivity_ of a mesh; however, this
// datastructure further includes some geometric data, indexing logic, etc.
//
// Each element (Vertex/Face/Edge/Halfedge) has an _index_ associated with it, corresponding to the canonical
// indexing scheme on the mesh initially passed in by the user. Note that (for instance), if Polyscope triangulates the
// mesh, newly introduced edges have index == INVALID_IND, as they correspond to nothing in the user's world.
class HalfedgeMesh {

public:
  // === Mesh members ===

  // Forward declarations
  class Vertex;
  class Edge;
  class Face;

  class Halfedge {
    friend class HalfedgeMesh;

  public:
    // Note: if mesh came from triangulation, refers to original mesh
    inline size_t index() const { return index_; }
    inline bool hasValidIndex() const { return index_ != INVALID_IND; }
    inline bool isReal() const { return isReal_; }
    inline Halfedge& twin() { return *twin_; }
    inline Halfedge& next() { return *next_; }
    inline Vertex& vertex() { return *vertex_; }
    inline Face& face() { return *face_; }
    inline Edge& edge() { return *edge_; }


  private:
    // Connectivity
    size_t index_;
    bool isReal_;
    Halfedge* twin_;
    Halfedge* next_;
    Vertex* vertex_;
    Face* face_;
    Edge* edge_;

    // Geometry
  };

  class Vertex {
    friend class HalfedgeMesh;

  public:
    inline size_t index() const { return index_; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline bool isBoundary() const { return isBoundary_; }
    inline size_t degree() const { return degree_; }

    inline glm::vec3 position() const { return position_; }
    inline glm::vec3 normal() const { return normal_; }
    inline double area() const { return area_; }

  private:
    // Connectivity
    size_t index_;
    Halfedge* halfedge_;
    bool isBoundary_;
    size_t degree_;

    // Geometry
    glm::vec3 position_;
    glm::vec3 normal_;
    double area_;
  };

  class Face {
    friend class HalfedgeMesh;

  public:
    // Note: if mesh came from triangulation, refers to original mesh
    inline size_t index() const { return index_; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline size_t nSides() const { return nSides_; }
    inline bool isReal() const { return isReal_; }

    // Common-case helper. Only meaningul if face is triangular.
    inline std::array<Vertex*, 3> triangleVertices() const { return triangleVertices_; }

    inline glm::vec3 normal() const { return normal_; }
    inline glm::vec3 center() const { return center_; }
    inline double area() const { return area_; }


  private:
    // Connectivity
    size_t index_;
    Halfedge* halfedge_;
    size_t nSides_;
    bool isReal_;
    std::array<Vertex*, 3> triangleVertices_;

    // Geometry
    glm::vec3 normal_;
    glm::vec3 center_;
    double area_;
  };


  class Edge {
    friend class HalfedgeMesh;

  public:
    // Note: if mesh came from triangulation, refers to original mesh
    inline size_t index() const { return index_; }
    inline bool hasValidIndex() const { return index_ != INVALID_IND; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline bool isBoundary() const { return isBoundary_; }
    inline double length() const { return length_; }

  private:
    // Connectivity
    size_t index_;
    Halfedge* halfedge_;
    bool isBoundary_;

    // Geometry
    double length_;
  };

  HalfedgeMesh();
  HalfedgeMesh(const std::vector<glm::vec3> vertexPositions, const std::vector<std::vector<size_t>> faceInds,
               bool triangulate = false);


  // Number of mesh elements of each type
  size_t nHalfedges() const { return halfedges.size(); }
  size_t nVertices() const { return vertices.size(); }
  size_t nEdges() const { return edges.size(); }
  size_t nFaces() const { return faces.size(); }
  size_t nBoundaryLoops() const { return boundaryLoops.size(); }
  size_t nImaginaryHalfedges() const { return halfedges.size() - nRealHalfedges(); }
  size_t nRealHalfedges() const { return nRealHalfedges_; }

  // If this mesh is a triangulation of the input, the number of elements in the original input mesh.
  size_t nOrigFaces() const { return nOrigFaces_; }
  size_t nOrigEdges() const { return nOrigEdges_; }
  size_t nOrigHalfedges() const { return nOrigHalfedges_; }

  // Utility functions
  bool isTriangular() const;
  int eulerCharacteristic() const;
  size_t nConnectedComponents() const;
  void updateVertexPositions(const std::vector<glm::vec3>& newPositions);

  // The contiguous chunks of memory which hold the actual structs.
  // Don't modify them after construction.
  std::vector<Halfedge> halfedges; // first real, then imaginary
  std::vector<Vertex> vertices;
  std::vector<Edge> edges;
  std::vector<Face> faces;
  std::vector<Face> boundaryLoops;

private:
  // Hide copy and move constructors, we don't wanna mess with that
  HalfedgeMesh(const HalfedgeMesh& other) = delete;
  HalfedgeMesh& operator=(const HalfedgeMesh& other) = delete;
  HalfedgeMesh(HalfedgeMesh&& other) = delete;
  HalfedgeMesh& operator=(HalfedgeMesh&& other) = delete;

  // Cache some basic information that may be queried many
  // times, but require O(n) computation to determine.
  void cacheInfo();
  void cacheGeometry();
  void cache_isSimplicial();
  void cache_nConnectedComponents();
  bool _isSimplicial;
  size_t _nConnectedComponents;
  size_t nOrigFaces_, nOrigEdges_, nOrigHalfedges_;
  size_t nRealHalfedges_;
};

} // namespace polyscope

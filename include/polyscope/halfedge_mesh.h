#pragma once

#include <vector>
#include <array>

#include <polyscope/utilities.h>

namespace polyscope {

// A HalfedgeMesh encodes the connectivity---but not the geometry---of a
// manifold surface, possibly with boundary.


class HalfedgeMesh {
public:
  // === Mesh members ===

  class Vertex;
  class Edge;
  class Face;

  class Halfedge {
    friend class HalfedgeMesh;

  public:
    // Note: if mesh came from triangulation, refers to original mesh
    inline size_t index() const { return index_; }
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
  size_t nImaginaryHalfedges() const; // TODO

  // If this mesh is a triangulation of the input, the number of elements in the original input mesh.
  size_t nOrigFaces() const;
  size_t nOrigEdges() const;
  size_t nOrigHalfedges() const;

  // Utility functions
  bool isTriangular() const;
  int eulerCharacteristic() const;
  size_t nConnectedComponents() const;

  // The contiguous chunks of memory which hold the actual structs.
  // Don't modify them after construction.
  size_t nRealHalfedges;
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
  void cache_nFacesTriangulation();
  void cache_longestBoundaryLoop();
  void cache_nConnectedComponents();
  bool _isSimplicial;
  size_t _nFacesTriangulation;
  size_t _longestBoundaryLoop;
  size_t _nConnectedComponents;
  size_t nOrigFaces_, nOrigEdges_, nOrigHalfedges_;
};

} // namespace polyscope

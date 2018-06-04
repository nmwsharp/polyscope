#pragma once

#include <vector>

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
    inline size_t index() { return index_; }
    inline bool isReal() { return isReal_; }
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
    inline size_t index() { return index_; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline bool isBoundary() { return isBoundary_; }
    inline size_t degree() { return degree_; }

    inline glm::vec3 position() { return position_; }
    inline glm::vec3 normal() { return normal_; }
    inline double area() { return area_; }

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
    inline size_t index() { return index_; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline size_t nSides() { return nSides_; }
    inline bool isReal() { return isReal_; }

    inline glm::vec3 normal() { return normal_; }
    inline double area() { return area_; }


  private:
    // Connectivity
    size_t index_;
    Halfedge* halfedge_;
    size_t nSides_;
    bool isReal_;

    // Geometry
    glm::vec3 normal_;
    double area_;
  };


  class Edge {
    friend class HalfedgeMesh;

  public:
    inline size_t index() { return index_; }
    inline Halfedge& halfedge() { return *halfedge_; }
    inline bool isBoundary() { return isBoundary_; }
    inline double length() { return length_; }

  private:
    // Connectivity
    size_t index_;
    Halfedge* halfedge_;
    bool isBoundary_;

    // Geometry
    double length_;
  };

  HalfedgeMesh();
  HalfedgeMesh(const std::vector<glm::vec3>& vertexPositions, const std::vector<std::vector<size_t>>& faceInds);


  // Number of mesh elements of each type
  size_t nHalfedges() const;
  size_t nVertices() const;
  size_t nEdges() const;
  size_t nFaces() const;
  size_t nBoundaryLoops() const;
  size_t nImaginaryHalfedges() const;

  // Utility functions
  bool isTriangular() const;          // returns true if and only if all faces are triangles
  size_t nFacesTriangulation() const; // returns the number of triangles in the
                                      // triangulation determined by
                                      // Face::triangulate()

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
};

} // namespace polyscope

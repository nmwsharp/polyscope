#include "polyscope/halfedge_mesh.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "polyscope/disjoint_sets.h"
#include "polyscope/messages.h"

using std::cout;
using std::endl;

namespace polyscope {

// Cache some basic information that may be queried many
// times, but require O(n) computation to determine.
void HalfedgeMesh::cacheInfo() {
  cache_isSimplicial();
  cache_nFacesTriangulation();
  cache_longestBoundaryLoop();
  cache_nConnectedComponents();
}

// Returns true if and only if all faces are triangles
bool HalfedgeMesh::isTriangular() const { return _isSimplicial; }

void HalfedgeMesh::cache_isSimplicial() {
  _isSimplicial = true;
  for (Face& f : faces) {
    if (f.nSides() != 3) {
      _isSimplicial = false;
      return;
    }
  }
}

void HalfedgeMesh::cache_nConnectedComponents() {

  DisjointSets dj(nVertices());
  for (Edge& e : edges) {
    dj.merge(e.halfedge().vertex().index(), e.halfedge().twin().vertex().index());
  }
  std::unordered_set<size_t> distinctComponents;
  for (size_t i = 0; i < nVertices(); i++) {
    distinctComponents.insert(dj.find(i));
  }
  _nConnectedComponents = distinctComponents.size();
}


int HalfedgeMesh::eulerCharacteristic() const {
  // be sure to do intermediate arithmetic with large, signed integers
  return static_cast<int>(static_cast<long long int>(nVertices()) - static_cast<long long int>(nEdges()) +
                          static_cast<long long int>(nFaces()));
}


size_t HalfedgeMesh::nConnectedComponents() const { return _nConnectedComponents; }

HalfedgeMesh::HalfedgeMesh() : _isSimplicial(true), _nFacesTriangulation(0), _longestBoundaryLoop(0) {}

// Helpers for below
namespace {
size_t halfedgeLookup(const std::vector<size_t>& compressedList, size_t target, size_t start, size_t end) {
  // Linear search is fast for small searches
  if (end - start < 20) {
    for (size_t i = start; i < end; i++) {
      if (compressedList[i] == target) {
        return i;
      }
    }
    return std::numeric_limits<size_t>::max();
  }
  // ...but we don't want to degrade to O(N^2) for really high valence vertices,
  // so fall back to a binary search
  else {
    auto loc = std::lower_bound(compressedList.begin() + start, compressedList.begin() + end, target);

    if (loc != (compressedList.begin() + end) && (target == *loc)) {
      return loc - compressedList.begin();
    } else {
      return std::numeric_limits<size_t>::max();
    }
  }
}
} // namespace

HalfedgeMesh::HalfedgeMesh(const std::vector<glm::vec3> vertexPositions,
                           const std::vector<std::vector<size_t>> faceInds, bool triangulate) {


  // == Optionally triangulate
  // For each face and halfedge, track the index of the original face (or halfedge) that it came from. For halfedges,
  // newly created halfedges are indicated by INVALID_IND
  std::vector<size_t> origFaceInd;
  std::vector<std::vector<size_t>> origHalfedgeInd;
  nOrigFaces_ = faceInds.size();
  if (triangulate) {

    std::vector<std::vector<size_t>> triangulatedFaces;
    size_t iHe = 0;

    // Iterate through the input polygons
    for (size_t iOrigFace = 0; iOrigFace < faceInds.size(); iOrigFace++) {

      // Fan-triangulate each polygon
      const std::vector<size_t>& origFace = faceInds[iOrigFace];
      for (size_t jE = 2; jE < origFace.size(); jE++) {
        size_t v0 = origFace[0];
        size_t v1 = origFace[jE - 1];
        size_t v2 = origFace[jE];

        // Add a new triangle
        triangulatedFaces.push_back({v0, v1, v2});

        // Track the original face indices and original halfedge indices
        origFaceInd.push_back(iOrigFace);
        origHalfedgeInd.push_back({iHe, iHe + jE - 1, iHe + jE});
      }

      iHe += origFace.size();
    }

    nOrigHalfedges_ = iHe;
  }
  // Not triangulating; push identity maps to preserve original indices
  else {

    // Identity face index
    origFaceInd.resize(faceInds.size());
    for (size_t i = 0; i < origFaceInd.size(); i++) {
      origFaceInd[i] = i;
    }

    // Identity halfedge index
    origHalfedgeInd = faceInds;
    size_t iHe = 0;
    for (std::vector<size_t>& vec : origHalfedgeInd) {
      for (size_t& ind : vec) {
        ind = iHe++;
      }
    }
    nOrigHalfedges_ = iHe;
  }


  /*   High-level outline of this algorithm:
   *
   *      0. Count how many of each object we will need so we can pre-allocate
   * them
   *
   *      1. Iterate over the faces of the input mesh, creating edge, face, and
   * halfedge objects
   *
   *      2. Walk around boundaries, marking boundary edge and creating
   * imaginary halfedges/faces
   *
   *      3. Copy the vertex positions to the geometry associated with the new
   * mesh
   *
   */

  // === 0. Count needed objects

  // === 0.5 Efficiently build an adjacent-face lookup table

  // Build a sorted list of (directed halfedge) neighbors of each vertex in
  // compressed format.

  // Count neighbors of each vertex
  size_t nDirected = 0;
  size_t maxPolyDegree = 0;
  std::vector<size_t> vertexNeighborsCount(vertexPositions.size(), 0);
  std::vector<size_t> vertexNeighborsStart(vertexPositions.size() + 1);
  for (size_t iFace = 0; iFace < faceInds.size(); iFace++) {
    auto poly = faceInds[iFace];
    nDirected += poly.size();
    maxPolyDegree = std::max(maxPolyDegree, poly.size());
    for (size_t j : poly) {
      vertexNeighborsCount[j]++;
    }
  }

  // Build a running sum of the number of neighbors to use a compressed list
  vertexNeighborsStart[0] = 0;
  size_t runningSum = 0;
  for (size_t iVert = 0; iVert < vertexPositions.size(); iVert++) {
    runningSum += vertexNeighborsCount[iVert];
    vertexNeighborsStart[iVert + 1] = runningSum;
  }

  // Populate the compressed list
  // Each vertex's neighbors are stored between vertexNeighborsStart[i] and
  // vertexNeighborsStart[i+1]
  std::vector<size_t> vertexNeighborsInd(vertexNeighborsStart.begin(), vertexNeighborsStart.end() - 1);
  std::vector<size_t> allVertexNeighbors(nDirected);
  for (size_t iFace = 0; iFace < faceInds.size(); iFace++) {
    auto poly = faceInds[iFace];
    for (size_t j = 0; j < poly.size(); j++) {
      size_t fromInd = poly[j];
      size_t toInd = poly[(j + 1) % poly.size()];
      allVertexNeighbors[vertexNeighborsInd[fromInd]] = toInd;
      vertexNeighborsInd[fromInd]++;
    }
  }

  // Sort each of the sublists in the compressed list
  for (size_t iVert = 0; iVert < vertexPositions.size(); iVert++) {
    std::sort(allVertexNeighbors.begin() + vertexNeighborsStart[iVert],
              allVertexNeighbors.begin() + vertexNeighborsStart[iVert + 1]);
  }

  // Count real and imaginary edges and faces and cache adjacent twin indices
  // Note: counting boundary loops is kinda difficult, so we wait to do so until
  // the final step
  size_t nPairedEdges = 0;
  size_t nUnpairedEdges = 0;
  std::vector<size_t> twinInd(allVertexNeighbors.size());
  for (size_t iVert = 0; iVert < vertexPositions.size(); iVert++) {
    size_t jStart = vertexNeighborsStart[iVert];
    size_t jEnd = vertexNeighborsStart[iVert + 1];
    for (size_t jInd = jStart; jInd < jEnd; jInd++) {
      size_t jVert = allVertexNeighbors[jInd];

      // Search for the j --> i edge
      size_t searchResult =
          halfedgeLookup(allVertexNeighbors, iVert, vertexNeighborsStart[jVert], vertexNeighborsStart[jVert + 1]);
      twinInd[jInd] = searchResult;
      if (searchResult == INVALID_IND) {
        nUnpairedEdges++;
      } else {
        nPairedEdges++;
      }
    }
  }
  nPairedEdges /= 2;

  size_t nTotalEdges = nPairedEdges + nUnpairedEdges;
  nRealHalfedges = 2 * nPairedEdges + nUnpairedEdges;
  size_t nImaginaryHalfedges = nUnpairedEdges;
  size_t nRealFaces = faceInds.size();

  // Allocate space
  halfedges.resize(nRealHalfedges + nImaginaryHalfedges);
  vertices.resize(vertexPositions.size());
  edges.resize(nTotalEdges);
  faces.resize(nRealFaces);

  // === 1. Create faces, edges, and halfedges

  // Keep track of the edges we've already created since we only need one per
  // edge
  std::vector<Edge*> sharedEdges(twinInd.size(), nullptr);

  // Iterate over faces
  size_t iFace = 0;
  size_t iEdge = 0;
  size_t iHalfedge = 0;
  std::vector<Halfedge*> thisFaceHalfedges;
  thisFaceHalfedges.reserve(maxPolyDegree);
  for (auto poly : faceInds) {
    size_t degree = poly.size();

    // Create a new face object
    Face& f = faces[iFace];
    f.isReal_ = true;

    // The halfedges that make up this face
    // std::vector<HalfedgePtr> thisFaceHalfedges(degree);
    thisFaceHalfedges.resize(degree);

    for (size_t iPolyEdge = 0; iPolyEdge < degree; iPolyEdge++) {
      Halfedge& he = halfedges[iHalfedge];
      iHalfedge++;

      size_t ind1 = poly[iPolyEdge];
      size_t ind2 = poly[(iPolyEdge + 1) % degree];

      // Connect up pointers
      he.vertex_ = &vertices[ind1];
      he.vertex().halfedge_ = &he;
      he.face_ = &f;
      thisFaceHalfedges[iPolyEdge] = &he;
      he.twin_ = nullptr; // ensure this is null so we can detect boundaries below

      // Get a reference to the edge shared by this and its twin, creating the
      // object if needed
      size_t myHeInd =
          halfedgeLookup(allVertexNeighbors, ind2, vertexNeighborsStart[ind1], vertexNeighborsStart[ind1 + 1]);
      size_t twinHeInd = twinInd[myHeInd];
      bool edgeAlreadyCreated = (twinHeInd != INVALID_IND) && (sharedEdges[twinHeInd] != nullptr);

      if (edgeAlreadyCreated) {
        Edge& sharedEdge = *sharedEdges[twinHeInd];
        he.edge_ = &sharedEdge;
        he.twin_ = &sharedEdge.halfedge();
        he.twin().twin_ = &he;
      } else {
        Edge& sharedEdge = edges[iEdge];
        iEdge++;
        sharedEdge.halfedge_ = &he;
        he.edge_ = &sharedEdge;
        sharedEdges[myHeInd] = &sharedEdge;
      }
    }

    // Do one more lap around the face to set next pointers
    for (size_t iPolyEdge = 0; iPolyEdge < degree; iPolyEdge++) {
      thisFaceHalfedges[iPolyEdge]->next_ = thisFaceHalfedges[(iPolyEdge + 1) % degree];
    }

    f.halfedge_ = thisFaceHalfedges[0];
    thisFaceHalfedges.clear();
    iFace++;
  }

  // === 2. Walk the boundary to find/create boundary cycles

  // First, do a pre-walk to count the boundary loops we will need and allocate
  // them
  size_t nBoundaryLoops = 0;
  std::set<Halfedge*> walkedHalfedges;
  for (size_t iHe = 0; iHe < nHalfedges(); iHe++) {
    if (halfedges[iHe].twin_ == nullptr && walkedHalfedges.find(&halfedges[iHe]) == walkedHalfedges.end()) {
      nBoundaryLoops++;
      Halfedge* currHe = &halfedges[iHe];
      walkedHalfedges.insert(currHe);
      size_t walkCount = 0;
      do {
        currHe = &currHe->next();
        while (currHe->twin_ != nullptr) {
          currHe = &currHe->twin().next();
          walkCount++;
          if (walkCount > nHalfedges()) {
            throw std::runtime_error(
                "Encountered infinite loop while constructing halfedge mesh. Are you sure the input is manifold?");
          }
        }
        walkedHalfedges.insert(currHe);
      } while (currHe != &halfedges[iHe]);
    }
  }
  boundaryLoops.resize(nBoundaryLoops);

  // Now do the actual walk in which we construct and connect objects
  size_t iBoundaryLoop = 0;
  size_t iImaginaryHalfedge = 0;
  for (size_t iHe = 0; iHe < nHalfedges(); iHe++) {
    // Note: If distinct holes share a given vertex, this algorithm will see
    // them as a single "figure 8-like"
    // hole and connect them with a single imaginary face.
    // TODO: fix this?

    // If this halfedge doesn't have a twin, it must be on a boundary (or have
    // already been processed while walking a hole)
    if (halfedges[iHe].twin_ == nullptr) {
      // Create a boundary loop for this hole
      Face& boundaryLoop = boundaryLoops[iBoundaryLoop];
      boundaryLoop.isReal_ = false;

      // Walk around the boundary loop, creating imaginary halfedges
      Halfedge* currHe = &halfedges[iHe];
      Halfedge* prevHe = nullptr;
      bool finished = false;
      while (!finished) {
        // Create a new, imaginary halfedge
        Halfedge& newHe = halfedges[nRealHalfedges + iImaginaryHalfedge];
        boundaryLoop.halfedge_ = &newHe;
        iImaginaryHalfedge++;

        // Connect up pointers
        newHe.isReal_ = false;
        newHe.twin_ = currHe;
        currHe->twin_ = &newHe;
        newHe.face_ = &boundaryLoop;
        newHe.edge_ = &currHe->edge();
        newHe.vertex_ = &currHe->next().vertex();

        // Some pointers need values only visible from the previous iteration of the loop.
        // The first one we process gets missed; handle it at the end outside the loop.
        if (prevHe == nullptr) {
          newHe.next_ = nullptr;
        } else {
          newHe.next_ = &prevHe->twin();
        }

        // Set the isBoundary property where appropriate
        currHe->vertex().isBoundary_ = true;
        currHe->edge().isBoundary_ = true;

        // Prepare for the next iteration
        prevHe = currHe;
        currHe = currHe->next_;
        while (currHe->twin_ != nullptr) {
          // When we've finished walking the loop, we'll be able to tell
          // because we spin in circles around the vertex trying to continue
          // the loop. Detect that here and quit.
          if (currHe->twin().next_ == nullptr) {
            finished = true;
            break;
          }

          currHe = &currHe->twin().next();
        }
      }

      // As noted above, the pointers don't get set properly on the first iteration of the loop above because we don't
      // have a reference to prev yet. Fix that here.
      halfedges[iHe].twin().next_ = &prevHe->twin();

      iBoundaryLoop++;
    }
  }

  // === 3. Map vertices in the halfedge mesh to the associated vertex
  for (size_t iV = 0; iV < vertices.size(); iV++) {
    vertices[iV].position_ = vertexPositions[iV];
  }

  // === 4. Set indices
  for (size_t iV = 0; iV < nVertices(); iV++) {
    vertices[iV].index_ = iV;
  }

  // Zero out edge indices to set below
  for (size_t iE = 0; iE < nEdges(); iE++) {
    edges[iE].index_ = INVALID_IND;
  }

  size_t currEdgeInd = 0;
  for (size_t iF = 0; iF < nFaces(); iF++) {
    faces[iF].index_ = origFaceInd[iF];

    Halfedge* currHe = &faces[iF].halfedge();
    std::vector<size_t>& origPolyHalfedgeInds = origHalfedgeInd[iF];
    for (size_t ind : origPolyHalfedgeInds) {
      currHe->index_ = ind;
      Edge& edge = currHe->edge();

      // If this edge was not induced by triangulation, and we haven't indexed the edge yet, then index it
      if (ind != INVALID_IND && edge.index_ == INVALID_IND) {
        edge.index_ = currEdgeInd++;
      }

      currHe = &currHe->next();
    }
  }
  nOrigEdges_ = currEdgeInd;


  // Count face sides, cache triangle vertices
  for (size_t iF = 0; iF < nFaces(); iF++) {
    Face& face = faces[iF];

    face.nSides_ = 0;
    Halfedge* currHe = &face.halfedge();
    Halfedge* firstHe = &face.halfedge();
    do {
      if (face.nSides_ < 3) {
        face.triangleVertices_[face.nSides_] = &currHe->vertex();
      }
      face.nSides_++;
      currHe = &currHe->next();
    } while (currHe != firstHe);
  }

  // Count vertex degree
  for (size_t iV = 0; iV < nVertices(); iV++) {
    Vertex& vertex = vertices[iV];
    vertex.degree_ = 0;
    Halfedge* currHe = &vertex.halfedge();
    Halfedge* firstHe = &vertex.halfedge();
    do {
      vertex.degree_++;
      currHe = &currHe->twin().next();
    } while (currHe != firstHe);
  }

  // Print some nice statistics
  std::cout << "Constructed halfedge mesh with: " << std::endl;
  std::cout << "    # verts =  " << nVertices() << std::endl;
  std::cout << "    # edges =  " << nEdges() << std::endl;
  std::cout << "    # faces =  " << nFaces() << std::endl;
  std::cout << "    # halfedges =  " << nHalfedges() << std::endl;
  std::cout << "      and " << nBoundaryLoops << " boundary components. " << std::endl;

  // Compute some basic information about the mesh
  cacheInfo();

  // Cache geometric data
  cacheGeometry();
}

void HalfedgeMesh::cacheGeometry() {

  // TODO special logic for triangulated meshes

  // == Zero out quantities that will accumulate
  for (size_t iV = 0; iV < nVertices(); iV++) {
    vertices[iV].area_ = 0.0;
  }

  // Loop over faces to compute most quantities
  for (size_t iF = 0; iF < nFaces(); iF++) {
    Face& face = faces[iF];
    size_t nSides = face.nSides();

    // = First pass, compute average quantities for the face
    double areaSum = 0;
    glm::vec3 normalSum{0., 0., 0.};
    glm::vec3 centerSum{0., 0., 0.};
    Vertex *vert0, *vert1, *vert2;
    {
      Halfedge* currHe = &face.halfedge();
      for (size_t jE = 0; jE < nSides; jE++) {

        vert0 = vert1;
        vert1 = vert2;
        vert2 = &currHe->vertex();

        if (jE >= 2) {
          // (implicit fan triangulation)
          glm::vec3 vec01 = vert1->position() - vert0->position();
          glm::vec3 vec02 = vert2->position() - vert0->position();
          glm::vec3 areaNormal = glm::cross(vec01, vec02);

          double area = 0.5 * glm::length(areaNormal);
          areaSum += area;
          normalSum += areaNormal;
        }
        centerSum += vert2->position();

        currHe = &currHe->next();
      }

      face.area_ = areaSum;
      face.normal_ = glm::normalize(normalSum);
      face.center_ = centerSum / (float)nSides;
    }

    { // = Second pass, distribute average quantities to vertices
      Halfedge* currHe = &face.halfedge();
      Vertex *vPrev, *v, *vNext;
      for (size_t jV = 0; jV <= nSides; jV++) {

        vPrev = v;
        v = vNext;
        vNext = &currHe->vertex();

        if (jV > 0) {
          // Assign area to vertices evenly
          v->area_ += face.area() / nSides;

          // Angle-weighted vertex normals
          glm::vec3 vecPrev = glm::normalize(vPrev->position() - v->position());
          glm::vec3 vecNext = glm::normalize(vNext->position() - v->position());
          double angle = std::acos(glm::clamp(glm::dot(vecPrev, vecNext), -1.0f, 1.0f));
          v->normal_ += (float)angle * face.normal();

          // Edge lengths
          double eLen = glm::length(currHe->vertex().position() - currHe->twin().vertex().position());
          currHe->edge().length_ = eLen;
        }

        currHe = &currHe->next();
      }
    }
  }

  // Normalize vertex normals
  for (size_t iV = 0; iV < nVertices(); iV++) {
    vertices[iV].normal_ = glm::normalize(vertices[iV].normal_);
  }
}

void HalfedgeMesh::updateVertexPositions(const std::vector<glm::vec3>& newPositions) {

  if (newPositions.size() != nVertices()) {
    error("Attempted to update vertex positions with list whose size (" + std::to_string(newPositions.size()) +
          ") does not match number of vertices(" + std::to_string(nVertices()) + ")");
  }

  for (size_t i = 0; i < newPositions.size(); i++) {
    vertices[i].position_ = newPositions[i];
  }

  cacheGeometry();
}

} // namespace polyscope

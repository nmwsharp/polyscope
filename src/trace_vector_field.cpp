// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/trace_vector_field.h"

#include "glm/gtx/rotate_vector.hpp"

namespace polyscope {

// Helpers for tracing
namespace {

struct PointNormal {
  glm::vec3 p;
  glm::vec3 n;
};

struct FacePoint {
  size_t f;
  glm::vec3 baryWeights;
};

struct IntersectionResult {
  float tRay;
  float tLine;
};


IntersectionResult rayLineIntersection(glm::vec2 rayStart, glm::vec2 rayDir, glm::vec2 lineA, glm::vec2 lineB) {

  glm::vec2 v1 = rayStart - lineA;
  glm::vec2 v2 = lineB - lineA;
  glm::vec2 v3{-rayDir.y, rayDir.x};

  float cross21 = v2.x * v1.y - v2.y * v1.x;
  float tRay = cross21 / dot(v2, v3);
  float tLine = glm::dot(v1, v3) / dot(v2, v3);

  if (tRay < 0) {
    tRay = std::numeric_limits<float>::infinity();
  }


  return IntersectionResult{tRay, tLine};
}


glm::vec3 unitSum(glm::vec3 v) {
  float sum = v.x + v.y + v.z;
  return v / sum;
}



class FieldTracer {

public:
  // Core members
  SurfaceMesh& mesh;
  std::vector<glm::vec2> faceVectors; // disambiguated
  int nSym;

  // Parameters
  float maxLineLength;
  size_t maxFaceCount;

  // Cached geometric quantities
  std::vector<glm::vec2> vert1InFaceBasis, vert2InFaceBasis;
  float totalArea;

  // Cached connectivity data

  // Input should be identified (raised to power), not disambiguated
  FieldTracer(SurfaceMesh& mesh_, const std::vector<glm::vec2>& field, int nSym_ = 1) : mesh(mesh_), nSym(nSym_) {

    mesh.ensureHaveFaceTangentSpaces();
    mesh.ensureHaveManifoldConnectivity();

    // Prepare the field
    faceVectors.resize(mesh.nFaces());
    for (size_t iF = 0; iF < mesh.nFaces(); iF++) {
      std::complex<float> c{field[iF].x, field[iF].y};
      std::complex<float> cRoot = std::pow(c, 1.0f / nSym);
      faceVectors[iF] = glm::vec2{cRoot.real(), cRoot.imag()};
    }

    // Cache geometry quantities
    vert1InFaceBasis.resize(mesh.nFaces());
    vert2InFaceBasis.resize(mesh.nFaces());
    totalArea = 0;
    for (size_t iF = 0; iF < mesh.nFaces(); iF++) {
      std::vector<size_t>& face = mesh.faces[iF];

      totalArea += mesh.faceAreas[iF];

      // Face basis
      glm::vec3 X = mesh.faceTangentSpaces[iF][0];
      glm::vec3 Y = mesh.faceTangentSpaces[iF][1];

      // Find each of the vertices as a point in the basis
      // The first vertex is implicitly at (0,0)
      size_t v0 = face[0];
      size_t v1 = face[1];
      size_t v2 = face[2];
      glm::vec3 pos1 = mesh.vertices[v1] - mesh.vertices[v0];
      glm::vec3 pos2 = mesh.vertices[v2] - mesh.vertices[v0];
      glm::vec2 p1{dot(X, pos1), dot(Y, pos1)};
      glm::vec2 p2{dot(X, pos2), dot(Y, pos2)};

      // Save data
      vert1InFaceBasis[iF] = p1;
      vert2InFaceBasis[iF] = p2;
    }

    maxLineLength = std::sqrt(totalArea) * .5;
    maxFaceCount = static_cast<size_t>(std::ceil(std::sqrt(mesh.nFaces())));
  }

  glm::vec3 facePointInR3(FacePoint p) {

    size_t v0 = mesh.faces[p.f][0];
    size_t v1 = mesh.faces[p.f][1];
    size_t v2 = mesh.faces[p.f][2];

    return p.baryWeights[0] * mesh.vertices[v0] + p.baryWeights[1] * mesh.vertices[v1] +
           p.baryWeights[2] * mesh.vertices[v2];
  }


  glm::vec2 barycentricToR2(FacePoint p) {
    return p.baryWeights[1] * vert1InFaceBasis[p.f] + p.baryWeights[2] * vert2InFaceBasis[p.f];
  }

  // Trace a single line through the field
  // traceSign should be 1.0 or -1.0, useful for tracing lines backwards through field
  std::vector<std::array<glm::vec3, 2>> traceLine(FacePoint startPoint, glm::vec2 startDir, float traceSign = 1.0) {

    // Accumulate the result here
    std::vector<std::array<glm::vec3, 2>> points;

    // Add the initial point
    glm::vec3 initPoint = facePointInR3(startPoint);
    points.push_back({{initPoint, mesh.faceNormals[startPoint.f]}});

    // Trace!
    FacePoint currPoint = startPoint;
    glm::vec2 currDir = startDir;
    size_t nFaces = 0;
    float lengthRemaining = maxLineLength;
    size_t prevFace = INVALID_IND;
	size_t prevPrevFace = INVALID_IND;
    while (lengthRemaining > 0 && currPoint.f != prevPrevFace && nFaces < maxFaceCount) {

      nFaces++;
      size_t currFace = currPoint.f;

      // Keep track of the last two faces visited
      prevPrevFace = prevFace;
      prevFace = currFace;

      // Get the data in the basis for this face
      glm::vec2 v0{0, 0};
      glm::vec2 v1 = vert1InFaceBasis[currFace];
      glm::vec2 v2 = vert2InFaceBasis[currFace];
      glm::vec2 pointPos = barycentricToR2(currPoint);

      // Pick the best symmetric direction in the face
      glm::vec2 faceDir = faceVectors[currFace];
      glm::vec2 traceDir{0., 0.};
      float bestAlign = -std::numeric_limits<float>::infinity();
      float deltaRot = 2.0 * PI / nSym;
      for (int iSym = 0; iSym < nSym; iSym++) {

        float alignScore = dot(traceSign * faceDir, currDir);

        if (alignScore > bestAlign) {
          bestAlign = alignScore;
          traceDir = traceSign * faceDir;
        }

        faceDir = glm::rotate(faceDir, deltaRot);
      }

      // Find when we would exit each edge (if ever)
      IntersectionResult hit0 = rayLineIntersection(pointPos, traceDir, v0, v1);
      IntersectionResult hit1 = rayLineIntersection(pointPos, traceDir, v1, v2);
      IntersectionResult hit2 = rayLineIntersection(pointPos, traceDir, v2, v0);

      // Check which edge we would exit first
      size_t nextFace;
      size_t nextHe;
      size_t exitHe;
      unsigned int exitHeLocalIndex = -1;
      float tCross, tRay;
      if (hit0.tRay <= hit1.tRay && hit0.tRay <= hit2.tRay) {
        exitHeLocalIndex = 0;
        exitHe = mesh.halfedgeIndices[currFace][0];
        nextHe = mesh.twinHalfedge[exitHe];
        tCross = 1.0 - hit0.tLine;
        tRay = hit0.tRay;
      } else if (hit1.tRay <= hit0.tRay && hit1.tRay <= hit2.tRay) {
        exitHeLocalIndex = 1;
        exitHe = mesh.halfedgeIndices[currFace][1];
        nextHe = mesh.twinHalfedge[exitHe];
        tCross = 1.0 - hit1.tLine;
        tRay = hit1.tRay;
      } else if (hit2.tRay <= hit0.tRay && hit2.tRay <= hit1.tRay) {
        exitHeLocalIndex = 2;
        exitHe = mesh.halfedgeIndices[currFace][2];
        nextHe = mesh.twinHalfedge[exitHe];
        tCross = 1.0 - hit2.tLine;
        tRay = hit2.tRay;
      } else {
        // cerr << "tracing failure :(" << endl;
        return points;
      }


      // If the ray would end before exiting the face, end it
      if (tRay > lengthRemaining) {
        tRay = lengthRemaining;
        glm::vec2 endingPos = pointPos + tRay * traceDir;
        glm::vec3 endingPosR3 = mesh.vertices[mesh.faces[currFace][0]] +
                                endingPos.x * mesh.faceTangentSpaces[currFace][0] +
                                endingPos.y * mesh.faceTangentSpaces[currFace][1];
        points.push_back({{endingPosR3, mesh.faceNormals[currFace]}});
        break;
      }

      // Generate a point for this intersection
      glm::vec2 newPointLocal = pointPos + tRay * traceDir;
      glm::vec3 newPointR3 = mesh.vertices[mesh.faces[currFace][0]] +
                             newPointLocal.x * mesh.faceTangentSpaces[currFace][0] +
                             newPointLocal.y * mesh.faceTangentSpaces[currFace][1];
      glm::vec3 newNormal = mesh.faceNormals[currFace];
      if (nextHe != INVALID_IND) {
        nextFace = mesh.faceForHalfedge[nextHe];
        newNormal = glm::normalize(newNormal + mesh.faceNormals[nextFace]);
      }
      points.push_back({{newPointR3, newNormal}});


      // Quit if we hit a boundary
      if (nextHe == INVALID_IND) {
        break;
      }


      // Subtract off the length expended in this face
      lengthRemaining -= tRay;


      // Find a direction of travel in the new face
      currDir = rotateToTangentBasis(traceDir, mesh.faceTangentSpaces[currFace][0], mesh.faceTangentSpaces[currFace][1],
                                     mesh.faceTangentSpaces[nextFace][0], mesh.faceTangentSpaces[nextFace][1]);

      // Figure out which halfedge in the next face is nextHe
      unsigned int nextHeLocalIndex = 0;
      while (mesh.halfedgeIndices[nextFace][nextHeLocalIndex] != nextHe) {
        nextHeLocalIndex++;
      }

      // On a non-manifold / not oriented mesh, the halfedges we're transitioning between might actually point the same
      // way. If so, flip tCross.
      size_t currTailVert = mesh.faces[currFace][exitHeLocalIndex];
      size_t nextTailVert = mesh.faces[nextFace][nextHeLocalIndex];
      if (currTailVert == nextTailVert) {
        tCross = 1.0 - tCross;
      }

      // Clamp for safety
      tCross = std::fmin(tCross, 1.0);
      tCross = std::fmax(tCross, 0.);

      // Find barycentric coordinates in the new face
      currPoint = FacePoint{nextFace, {0, 0, 0}};
      currPoint.baryWeights[nextHeLocalIndex] = 1.0 - tCross;
      currPoint.baryWeights[(nextHeLocalIndex + 1) % 3] = tCross;


      // Pull the result slightly towards the center of the new face to minimize numerical difficulties
      currPoint.baryWeights = unitSum(10000.f * currPoint.baryWeights + glm::vec3{1, 1, 1} / 3.f);
    }


    return points;
  }
};


}; // namespace

// Rotate in to a new basis in R3. Vector is rotated in to new tangent plane, then a change of basis is performed to
// the new basis. Basis vectors MUST be unit and orthogonal -- this function doesn't check.
glm::vec2 rotateToTangentBasis(glm::vec2 v, const glm::vec3& oldBasisX, const glm::vec3& oldBasisY,
                               const glm::vec3& newBasisX, const glm::vec3& newBasisY) {

  glm::vec3 oldNormal = glm::cross(oldBasisX, oldBasisY);
  glm::vec3 newNormal = glm::cross(newBasisX, newBasisY);

  // If the vectors are nearly in plane, no rotation is needed
  // (can't just go through the same code path as below because cross
  // yields a degenerate direction)
  float EPS = 0.0000001;
  float dotVal = dot(oldNormal, newNormal);
  glm::vec3 oldBasisInPlaneX, oldBasisInPlaneY;
  if (dotVal > (1.0 - EPS)) {
    // No rotation
    oldBasisInPlaneX = oldBasisX;
    oldBasisInPlaneY = oldBasisY;
  } else if (dotVal < -(1.0 - EPS)) {
    // 180 degree rotation
    oldBasisInPlaneX = -oldBasisX;
    oldBasisInPlaneY = -oldBasisY;
  } else {
    // general rotation
    glm::vec3 edgeV = glm::normalize(cross(oldNormal, newNormal));
    float angle = atan2(dot(edgeV, cross(oldNormal, newNormal)), dot(oldNormal, newNormal));
    oldBasisInPlaneX = glm::rotate(oldBasisX, angle, edgeV);
    oldBasisInPlaneY = glm::rotate(oldBasisY, angle, edgeV);
    // oldBasisInPlaneX = oldBasisX.rotate_around(edgeV, angle);
    // oldBasisInPlaneY = oldBasisY.rotate_around(edgeV, angle);
  }

  // Now it's just a goood old-fashioned change of basis
  float xComp = v.x * glm::dot(oldBasisInPlaneX, newBasisX) + v.y * glm::dot(oldBasisInPlaneY, newBasisX);
  float yComp = v.x * glm::dot(oldBasisInPlaneX, newBasisY) + v.y * glm::dot(oldBasisInPlaneY, newBasisY);
  return glm::vec2{xComp, yComp};
}

std::vector<std::vector<std::array<glm::vec3, 2>>> traceField(SurfaceMesh& mesh, const std::vector<glm::vec2>& field,
                                                              int nSym, size_t nLines) {

  // Only works on triangle meshes
  for (auto& face : mesh.faces) {
    if (face.size() != 3) {
      polyscope::warning("field tracing only supports triangular meshes");
      return std::vector<std::vector<std::array<glm::vec3, 2>>>();
    }
  }

  // Preliminaries

  // Create a tracer
  FieldTracer tracer(mesh, field, nSym);

  // Compute a reasonable number of lines if no count was specified
  if (nLines == 0) {
    float lineFactor = 10;
    nLines = static_cast<size_t>(std::ceil(lineFactor * std::sqrt(mesh.nFaces())));
  }

  // Shuffle the list of faces to get a reasonable distribution of starting points
  // Build a list of faces to start lines in. Unusually large faces get listed multiple times so we start more lines in
  // them. This roughly approximates a uniform sampling of the mesh. Small faces get oversampled, but that's much less
  // visually striking than large faces getting undersampled.
  std::vector<size_t> faceQueue;
  {
    float meanArea = tracer.totalArea / mesh.nFaces();
    for (size_t iF = 0; iF < mesh.nFaces(); iF++) {
      faceQueue.push_back(iF);
      float faceArea = mesh.faceAreas[iF];
      while (faceArea > meanArea) {
        faceQueue.push_back(iF);
        faceArea -= meanArea;
      }
    }

    // Shuffle the list (if we're tracing fewer lines than the size of the list, we want them to be distributed evenly)
    auto randomEngine = std::default_random_engine{};
    std::shuffle(faceQueue.begin(), faceQueue.end(), randomEngine);

    // Make sure the queue of faces to process is long enough by repeating it
    int iAppend = 0;
    while (faceQueue.size() < nLines) {
      faceQueue.push_back(faceQueue[iAppend++]);
    }
  }

  // Unit random numbers
  std::random_device device;
  std::mt19937 mt(device());
  std::uniform_real_distribution<double> unitDist(0.0, 1.0);
  auto unitRand = [&]() { return unitDist(mt); };


  // == Trace the lines
  // cout << "Tracing lines through vector field... " << endl;
  std::vector<std::vector<std::array<glm::vec3, 2>>> lineList;
  for (size_t i = 0; i < nLines; i++) {


    // Get the next starting face
    size_t startFace = faceQueue.back();
    faceQueue.pop_back();

    // Generate a random point in the face
    float r1 = unitRand();
    float r2 = unitRand();
    glm::vec3 randPoint{1.0 - std::sqrt(r1), std::sqrt(r1) * (1.0 - r2),
                        r2 * std::sqrt(r1)};                           // uniform sampling in triangle
    randPoint = unitSum(10000.f * randPoint + glm::vec3{1, 1, 1} / 3.f); // pull slightly towards center

    // trace half of lines backwards through field, reduces concentration near areas of convergence
    float traceSign = unitRand() > 0.5 ? 1.0 : -1.0;

    // Generate a random direction
    // (the tracing code snaps the velocity to the best-fitting direction, this just serves the role of picking
    // a random direction in symmetric fields)
    glm::vec2 randomDir = glm::normalize(glm::vec2{unitRand() - .5, unitRand() - .5});

    // Trace
    lineList.push_back(tracer.traceLine(FacePoint{startFace, randPoint}, randomDir, traceSign));
  }
  // cout << "    ... done tracing field." << endl;


  return lineList;
}

} // namespace polyscope

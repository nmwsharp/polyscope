// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/trace_vector_field.h"

namespace polyscope {

// Helpers for tracing
/* TODO
namespace {

struct PointNormal {
  Vector3 p;
  Vector3 n;
};

struct FacePoint {
  FacePtr f;
  Vector3 baryWeights;
};

struct IntersectionResult {
  double tRay;
  double tLine;
};


IntersectionResult rayLineIntersection(Vector2 rayStart, Vector2 rayDir, Vector2 lineA, Vector2 lineB) {

  Vector2 v1 = rayStart - lineA;
  Vector2 v2 = lineB - lineA;
  Vector2 v3{-rayDir.y, rayDir.x};

  double tRay = cross(v2, v1).z / dot(v2, v3);
  double tLine = dot(v1, v3) / dot(v2, v3);

  if (tRay < 0) {
    tRay = std::numeric_limits<double>::infinity();
  }


  return IntersectionResult{tRay, tLine};
}

int halfedgeIndex(HalfedgePtr he, FacePtr f) {

  if (he == f.halfedge()) return 0;
  if (he == f.halfedge().next()) return 1;
  if (he == f.halfedge().next().next()) return 2;

  return -7777777;
}


Vector3 unitSum(Vector3 v) {
  double sum = v.x + v.y + v.z;
  return v / sum;
}

// Rotate in to a new basis in R3. Vector is rotated in to new tangent plane, then a change of basis is performed to
// the new basis. Basis vectors MUST be unit and orthogonal -- this function doesn't check.
Vector2 rotateToTangentBasis(Vector2 v, const Vector3& oldBasisX, const Vector3& oldBasisY, const Vector3& newBasisX,
                             const Vector3& newBasisY) {

  Vector3 oldNormal = cross(oldBasisX, oldBasisY);
  Vector3 newNormal = cross(newBasisX, newBasisY);

  // If the vectors are nearly in plane, no rotation is needed
  // (can't just go through the same code path as below because cross
  // yields a degenerate direction)
  double EPS = 0.0000001;
  double dotVal = dot(oldNormal, newNormal);
  Vector3 oldBasisInPlaneX, oldBasisInPlaneY;
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
    Vector3 edgeV = unit(cross(oldNormal, newNormal));
    double angle = atan2(dot(edgeV, cross(oldNormal, newNormal)), dot(oldNormal, newNormal));
    oldBasisInPlaneX = oldBasisX.rotate_around(edgeV, angle);
    oldBasisInPlaneY = oldBasisY.rotate_around(edgeV, angle);
  }

  // Now it's just a goood old-fashioned change of basis
  double xComp = v.x * dot(oldBasisInPlaneX, newBasisX) + v.y * dot(oldBasisInPlaneY, newBasisX);
  double yComp = v.x * dot(oldBasisInPlaneX, newBasisY) + v.y * dot(oldBasisInPlaneY, newBasisY);
  return Vector2{xComp, yComp};
}


class FieldTracer {

public:
  // Core members
  Geometry<Euclidean>* geometry;
  HalfedgeMesh* mesh;
  GeometryCache<Euclidean>* gc;
  FaceData<Vector2> faceVectors; // disambiguated
  int nSym;

  // Parameters
  double maxLineLength;
  size_t maxFaceCount;

  // Cached geometric quantities
  FaceData<Vector2> vert1InFaceBasis, vert2InFaceBasis;
  double totalArea;


  // Input should be identified (raised to power), not disambiguated
  FieldTracer(Geometry<Euclidean>* geometry_, const FaceData<Complex>& field, int nSym_ = 1) {
    geometry = geometry_;
    mesh = geometry->getMesh();
    gc = &geometry->cache;
    nSym = nSym_;

    // Prepare the field
    faceVectors = FaceData<Vector2>(mesh);
    for (FacePtr f : mesh->faces()) {

      Complex disambigField = std::pow(field[f], 1.0 / nSym);
      faceVectors[f] = Vector2{disambigField.real(), disambigField.imag()};
    }

    // Cache geometry quantities
    gc->requireFaceBases();
    gc->requireFaceNormals();
    gc->requireFaceAreas();

    vert1InFaceBasis = FaceData<Vector2>(mesh);
    vert2InFaceBasis = FaceData<Vector2>(mesh);
    totalArea = 0;
    for (FacePtr f : mesh->faces()) {

      totalArea += gc->faceAreas[f];

      // Face basis
      Vector3 X = gc->faceBases[f][0];
      Vector3 Y = gc->faceBases[f][1];

      // Find each of the vertices as a point in the basis
      // The first vertex is implicitly at (0,0)
      VertexPtr v0 = f.halfedge().vertex();
      VertexPtr v1 = f.halfedge().next().vertex();
      VertexPtr v2 = f.halfedge().next().next().vertex();
      Vector3 pos1 = geometry->position(v1) - geometry->position(v0);
      Vector3 pos2 = geometry->position(v2) - geometry->position(v0);
      Vector2 p1{dot(X, pos1), dot(Y, pos1)};
      Vector2 p2{dot(X, pos2), dot(Y, pos2)};

      // Save data
      vert1InFaceBasis[f] = p1;
      vert2InFaceBasis[f] = p2;
    }

    maxLineLength = std::sqrt(totalArea) * .5;
    maxFaceCount = static_cast<size_t>(std::ceil(std::sqrt(mesh->nFaces())));
  }

  Vector3 facePointInR3(FacePoint p) {

    VertexPtr v1 = p.f.halfedge().vertex();
    VertexPtr v2 = p.f.halfedge().next().vertex();
    VertexPtr v3 = p.f.halfedge().next().next().vertex();

    return p.baryWeights[0] * geometry->position(v1) + p.baryWeights[1] * geometry->position(v2) +
           p.baryWeights[2] * geometry->position(v3);
  }


  Vector2 barycentricToR2(FacePoint p) {
    return p.baryWeights[1] * vert1InFaceBasis[p.f] + p.baryWeights[2] * vert2InFaceBasis[p.f];
  }

  // Trace a single line through the field
  // traceSign should be 1.0 or -1.0, useful for tracing lines backwards through field
  std::vector<std::array<Vector3, 2>> traceLine(FacePoint startPoint, Vector2 startDir, double traceSign=1.0) {

    // Accumulate the result here
    std::vector<std::array<Vector3, 2>> points;

    // Add the initial point
    Vector3 initPoint = facePointInR3(startPoint);
    points.push_back({{initPoint, gc->faceNormals[startPoint.f]}});

    // Trace!
    FacePoint currPoint = startPoint;
    Vector2 currDir = startDir;
    size_t nFaces = 0;
    double lengthRemaining = maxLineLength;
    FacePtr prevFace, prevPrevFace;
    while (lengthRemaining > 0 && currPoint.f != prevPrevFace && nFaces < maxFaceCount) {

      nFaces++;

      // Keep track of the last two faces visited
      prevPrevFace = prevFace;
      prevFace = currPoint.f;

      // Get the data in the basis for this face
      Vector2 v0{0, 0};
      Vector2 v1 = vert1InFaceBasis[currPoint.f];
      Vector2 v2 = vert2InFaceBasis[currPoint.f];
      Vector2 pointPos = barycentricToR2(currPoint);

      // Pick the best symmetric direction in the face
      Vector2 faceDir = faceVectors[currPoint.f];
      Vector2 traceDir;
      double bestAlign = -std::numeric_limits<double>::infinity();
      double deltaRot = 2.0 * PI / nSym;
      for (int iSym = 0; iSym < nSym; iSym++) {

        double alignScore = dot(traceSign * faceDir, currDir);

        if (alignScore > bestAlign) {
          bestAlign = alignScore;
          traceDir = traceSign * faceDir;
        }

        faceDir = faceDir.rotate(deltaRot);
      }

      // Find when we would exit each edge (if ever)
      IntersectionResult hit0 = rayLineIntersection(pointPos, traceDir, v0, v1);
      IntersectionResult hit1 = rayLineIntersection(pointPos, traceDir, v1, v2);
      IntersectionResult hit2 = rayLineIntersection(pointPos, traceDir, v2, v0);

      // Check which edge we would exit first
      FacePtr nextFace;
      HalfedgePtr crossingHalfedge;
      double tCross, tRay;
      if (hit0.tRay <= hit1.tRay && hit0.tRay <= hit2.tRay) {
        crossingHalfedge = currPoint.f.halfedge().twin();
        nextFace = crossingHalfedge.face();
        tCross = 1.0 - hit0.tLine;
        tRay = hit0.tRay;
      } else if (hit1.tRay <= hit0.tRay && hit1.tRay <= hit2.tRay) {
        crossingHalfedge = currPoint.f.halfedge().next().twin();
        nextFace = crossingHalfedge.face();
        tCross = 1.0 - hit1.tLine;
        tRay = hit1.tRay;
      } else if (hit2.tRay <= hit0.tRay && hit2.tRay <= hit1.tRay) {
        crossingHalfedge = currPoint.f.halfedge().next().next().twin();
        nextFace = crossingHalfedge.face();
        tCross = 1.0 - hit2.tLine;
        tRay = hit2.tRay;
      } else {
        cerr << "tracing failure :(" << endl;
        return points;
      }

      // If the ray would end before exiting the face, end it
      if (tRay > lengthRemaining) {
        tRay = lengthRemaining;
        Vector2 endingPos = pointPos + tRay * traceDir;
        Vector3 endingPosR3 = geometry->position(currPoint.f.halfedge().vertex()) +
                              endingPos.x * gc->faceBases[currPoint.f][0] + endingPos.y * gc->faceBases[currPoint.f][1];
        points.push_back({{endingPosR3, gc->faceNormals[currPoint.f]}});
        break;
      }


      // Generate a point for this intersection
      Vector3 newPoint = geometry->position(crossingHalfedge.vertex()) + geometry->vector(crossingHalfedge) * tCross;
      Vector3 newNormal = gc->faceNormals[currPoint.f];
      if (crossingHalfedge.isReal()) {
        newNormal = unit(newNormal + gc->faceNormals[nextFace]);
      }
      points.push_back({{newPoint, newNormal}});


      // Quit if we hit a boundary
      if (!crossingHalfedge.isReal()) {
        break;
      }


      // Subtract off the length expended in this face
      lengthRemaining -= tRay;


      // Find a direction of travel in the new face
      currDir = rotateToTangentBasis(traceDir, gc->faceBases[currPoint.f][0], gc->faceBases[currPoint.f][1],
                                     gc->faceBases[nextFace][0], gc->faceBases[nextFace][1]);

      // Find barycentric coordinates in the new face
      int iHe = halfedgeIndex(crossingHalfedge, nextFace);
      currPoint = FacePoint{nextFace, {0, 0, 0}};
      currPoint.baryWeights[iHe] = 1.0 - tCross;
      currPoint.baryWeights[(iHe + 1) % 3] = tCross;


      // Pull the result slightly towards the center of the new face to minimize numerical difficulties
      currPoint.baryWeights = unitSum(10000 * currPoint.baryWeights + Vector3{1, 1, 1} / 3);
    }


    return points;
  }
};


}; // namespace

*/

/*

std::vector<std::vector<std::array<glm::vec3, 2>>> traceField(HalfedgeMesh& mesh, const std::vector<Complex>& field,
                                                            int nSym, size_t nLines) {


// Preliminaries
HalfedgeMesh* mesh = geometry->getMesh();
GeometryCache<Euclidean>& gc = geometry->cache;
gc.requireFaceAreas();

// Create a tracer
FieldTracer tracer(geometry, field, nSym);

// Compute a reasonable number of lines if no count was specified
if (nLines == 0) {
  double lineFactor = 10;
  nLines = static_cast<size_t>(std::ceil(lineFactor * std::sqrt(mesh->nFaces())));
}

// Shuffle the list of faces to get a reasonable distribution of starting points
// Build a list of faces to start lines in. Unusually large faces get listed multiple times so we start more lines in
// them. This roughly approximates a uniform sampling of the mesh. Small faces get oversampled, but that's much less
// visually striking than large faces getting undersampled.
std::vector<FacePtr> faceQueue;
{
  double meanArea = tracer.totalArea / mesh->nFaces();
  for (FacePtr f : mesh->faces()) {
    faceQueue.push_back(f);
    double faceArea = gc.faceAreas[f];
    while (faceArea > meanArea) {
      faceQueue.push_back(f);
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


// == Trace the lines
//cout << "Tracing lines through vector field... " << endl;
std::vector<std::vector<std::array<Vector3, 2>>> lineList;
for (size_t i = 0; i < nLines; i++) {


  // Get the next starting face
  FacePtr startFace = faceQueue.back();
  faceQueue.pop_back();

  // Generate a random point in the face
  double r1 = unitRand();
  double r2 = unitRand();
  Vector3 randPoint{1.0 - std::sqrt(r1), std::sqrt(r1) * (1.0 - r2),
                    r2 * std::sqrt(r1)};                         // uniform sampling in triangle
  randPoint = unitSum(10000 * randPoint + Vector3{1, 1, 1} / 3); // pull slightly towards center
  double traceSign = unitRand() > 0.5 ? 1.0 : -1.0; // trace half of lines backwards through field, avoids
conentration near areas of convergence

  // Generate a random direction
  // (the tracing code snaps the velocity to the best-fitting direction, this just serves the role of picking
  // a random direction in symmetric fields)
  Vector2 randomDir = unit(Vector2{unitRand() - .5, unitRand() - .5});

  // Trace
  lineList.push_back(tracer.traceLine(FacePoint{startFace, randPoint}, randomDir, traceSign));
}
//cout << "    ... done tracing field." << endl;


std::vector<std::vector<std::array<glm::vec3, 2>>> lineList;
return lineList;
}
*/

} // namespace polyscope

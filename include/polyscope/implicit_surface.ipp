// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once


#include "polyscope/messages.h"
#include "polyscope/view.h"

#include <tuple>
#include <vector>

namespace polyscope {


template <class Func>
std::tuple<size_t, size_t, std::vector<float>, std::vector<glm::vec3>, std::vector<glm::vec3>>
renderImplictSurfaceFromCurrentView(Func&& func, ImplictRenderOpts opts) {

  // == Get current camera/image parameters
  if (view::projectionMode != ProjectionMode::Perspective) {
    // TODO to support orthographic, need to add view functions to get ray origins
    warning("implicit surface rendering only supports perspective projection");
    return std::tuple<size_t, size_t, std::vector<float>, std::vector<glm::vec3>, std::vector<glm::vec3>>{
        0, 0, std::vector<float>(), std::vector<glm::vec3>(), std::vector<glm::vec3>()};
  }

  // Read out option values
  const float missDist = opts.missDist.asAbsolute();
  const float hitDist = opts.hitDist.asAbsolute();
  const float stepFactor = opts.stepFactor;          // used for sphere march only
  const float stepSize = opts.stepSize.asAbsolute(); // used for fixed step only
  const size_t nMaxSteps = opts.nMaxSteps;
  const float normalSampleEps = opts.normalSampleEps;
  const ImplicitRenderMode mode = opts.mode;
  const int subsampleFactor = opts.subsampleFactor;


  glm::vec3 cameraLoc = view::getCameraWorldPosition();
  glm::mat4x4 viewMat = view::viewMat;
  size_t dimX = view::bufferWidth;
  size_t dimY = view::bufferHeight;
  size_t dimXsub = dimX / subsampleFactor;
  size_t dimYsub = dimY / subsampleFactor;
  size_t nPix = dimXsub * dimYsub;

  // Generate rays corresponding to each pixel
  // (this is a working set which will be shrunk as computation proceeds)
  std::vector<glm::vec3> rayRoots(nPix);
  std::vector<glm::vec3> rayDirs(nPix);
  std::vector<size_t> rayInds(nPix); // index of the ray
  for (size_t iY = 0; iY < dimYsub; iY++) {
    for (size_t iX = 0; iX < dimXsub; iX++) {
      size_t ind = iY * dimXsub + iX;
      rayRoots[ind] = cameraLoc;
      rayDirs[ind] = view::screenCoordsToWorldRay(glm::vec2{iX * subsampleFactor, iY * subsampleFactor});
      rayInds[ind] = ind;
    }
  }

  // Sample the first value at each ray (to check for sign changes)
  std::vector<float> currVals = func(rayRoots);
  std::vector<bool> initSigns(nPix);
  for (size_t iP = 0; iP < nPix; iP++) {
    initSigns[iP] = std::signbit(currVals[iP]);
  }

  // Write output data here

  // March along the ray to compute depth
  std::vector<float> rayDepth(nPix, 0.); // working data, gets shrunk and repacked
  std::vector<glm::vec3> currPos(nPix);
  std::vector<float> rayDepthOut(nPix, -1.);                        // output values
  std::vector<glm::vec3> rayPosOut(nPix, glm::vec3{0.f, 0.f, 0.f}); // output values
  size_t iFinished = 0;
  for (size_t iStep = 0; iStep < nMaxSteps; iStep++) {

    // Check for convergence & write/compact
    size_t iPack = 0;
    for (size_t iP = 0; iP < rayDepth.size(); iP++) {

      // Check for termination
      bool missTerminated = rayDepth[iP] > missDist;
      bool terminated =
          missTerminated || (std::abs(currVals[iP]) < hitDist) || (std::signbit(currVals[iP]) != initSigns[iP]);

      if (terminated) {
        // Write to the output buffer
        size_t outInd = rayInds[iP];
        glm::vec3 finalPos = rayRoots[iP] + rayDepth[iP] * rayDirs[iP];
        float outDepth = missTerminated ? -1.f : rayDepth[iP];
        rayDepthOut[outInd] = outDepth;
        rayPosOut[outInd] = finalPos;

        iFinished++;

      } else {
        // Take a step
        float rayStepSize;
        if (mode == ImplicitRenderMode::SphereMarch) {
          rayStepSize = std::abs(currVals[iP]) * stepFactor;
        } else if (mode == ImplicitRenderMode::FixedStep) {
          rayStepSize = stepSize;
        }

        float newDepth = rayDepth[iP] + rayStepSize;
        glm::vec3 newPos = rayRoots[iP] + newDepth * rayDirs[iP];

        // Write to the compacted array
        rayRoots[iPack] = rayRoots[iP];
        rayDirs[iPack] = rayDirs[iP];
        rayInds[iPack] = rayInds[iP];
        rayDepth[iPack] = newDepth;
        currPos[iPack] = newPos;
        iPack++;
      }
    }

    // "Trim" the working arrays to size
    rayRoots.resize(iPack);
    rayDirs.resize(iPack);
    rayInds.resize(iPack);
    rayDepth.resize(iPack);
    currPos.resize(iPack);

    // Evaluate the remaining rays
    currVals = func(currPos);
  }

  // == Compute normals
  // Uses finite differences on the vertices of a tetrahedron
  // (see https://iquilezles.org/articles/normalsSDF/)

  std::vector<glm::vec3> normalOut(nPix, glm::vec3{0.f, 0.f, 0.f}); // output values
  std::array<glm::vec3, 4> tetVerts({
      glm::vec3{1.f, -1.f, -1.f},
      glm::vec3{-1.f, -1.f, 1.f},
      glm::vec3{-1.f, 1.f, -1.f},
      glm::vec3{1.f, 1.f, 1.f},
  });

  currPos.resize(nPix);
  for (size_t iV = 0; iV < 4; iV++) {
    glm::vec3 vertVec = tetVerts[iV];

    // Set up the evaluation points for each pixel
    for (size_t iP = 0; iP < nPix; iP++) {
      float f = rayDepthOut[iP] * normalSampleEps;
      currPos[iP] = rayPosOut[iP] + f * vertVec;
    }

    // Evaluate the function at each sample point
    currVals = func(currPos);

    // Accumulate the result
    for (size_t iP = 0; iP < nPix; iP++) {
      normalOut[iP] += vertVec * currVals[iP];
    }
  }

  // Normalize the normal vectors and transform to view space
  glm::mat3x3 viewMat3(viewMat);
  for (size_t iP = 0; iP < nPix; iP++) {
    normalOut[iP] = viewMat3 * glm::normalize(normalOut[iP]);
  }

  // Handle not-converged rays
  for (size_t iP = 0; iP < nPix; iP++) {
    bool didConverge = rayDepthOut[iP] >= 0.;
    if (!didConverge) {
      rayDepthOut[iP] = std::numeric_limits<float>::infinity();
      normalOut[iP] = glm::vec3{0.f, 0.f, 0.f};
    }
  }


  return std::tuple<size_t, size_t, std::vector<float>, std::vector<glm::vec3>, std::vector<glm::vec3>>{
      dimXsub, dimYsub, rayDepthOut, rayPosOut, normalOut};
}

// =======================================================
// === Depth/geometry/shape only render functions
// =======================================================

template <class Func>
DepthRenderImage* renderImplictSurface(std::string name, Func&& func, ImplictRenderOpts opts) {
  return renderImplictSurface(getGlobalFloatingQuantityStructure(), name, func, opts);
}

template <class Func>
DepthRenderImage* renderImplictSurfaceBatch(std::string name, Func&& func, ImplictRenderOpts opts) {
  return renderImplictSurfaceBatch(getGlobalFloatingQuantityStructure(), name, func, opts);
}

template <class Func, class S>
DepthRenderImage* renderImplictSurface(QuantityStructure<S>* parent, std::string name, Func&& func,
                                       ImplictRenderOpts opts) {

  // Bootstrap on the batch version
  auto batchFunc = [&](std::vector<glm::vec3> inPos) {
    std::vector<float> outVals(inPos.size());
    for (size_t i = 0; i < inPos.size(); i++) {
      outVals[i] = func(inPos[i]);
    }
    return outVals;
  };

  return renderImplictSurfaceBatch(parent, name, batchFunc, opts);
}


template <class Func, class S>
DepthRenderImage* renderImplictSurfaceBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                            ImplictRenderOpts opts) {

  // Call the function which does all the hard work
  size_t dimXsub, dimYsub;
  std::vector<float> rayDepthOut;
  std::vector<glm::vec3> rayPosOut;
  std::vector<glm::vec3> normalOut;
  std::tie(dimXsub, dimYsub, rayDepthOut, rayPosOut, normalOut) = renderImplictSurfaceFromCurrentView(func, opts);

  // TODO check if there is an existing quantity of the same type/size to replace, and if so re-fill its buffers rather
  // than creating a whole new one

  // here, we bypass the conversion adaptor since we have explicitly filled matching types
  return parent->addDepthRenderImageImpl(name, dimXsub, dimYsub, rayDepthOut, normalOut);
}

// =======================================================
// === Colored surface render functions
// =======================================================


template <class Func, class FuncColor>
ColorRenderImage* renderImplictSurfaceColor(std::string name, Func&& func, FuncColor&& funcColor,
                                            ImplictRenderOpts opts) {
  return renderImplictSurfaceColor(getGlobalFloatingQuantityStructure(), name, func, funcColor, opts);
}

template <class Func, class FuncColor, class S>
ColorRenderImage* renderImplictSurfaceColorBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                 FuncColor&& funcColor, ImplictRenderOpts opts) {
  return renderImplictSurfaceColorBatch(getGlobalFloatingQuantityStructure(), name, func, funcColor, opts);
}


template <class Func, class FuncColor, class S>
ColorRenderImage* renderImplictSurfaceColor(QuantityStructure<S>* parent, std::string name, Func&& func,
                                            FuncColor&& funcColor, ImplictRenderOpts opts) {

  // Bootstrap on the batch version
  auto batchFunc = [&](std::vector<glm::vec3> inPos) {
    std::vector<float> outVals(inPos.size());
    for (size_t i = 0; i < inPos.size(); i++) {
      outVals[i] = func(inPos[i]);
    }
    return outVals;
  };

  auto batchFuncColor = [&](std::vector<glm::vec3> inPos) {
    std::vector<glm::vec3> outVals(inPos.size());
    for (size_t i = 0; i < inPos.size(); i++) {
      outVals[i] = funcColor(inPos[i]);
    }
    return outVals;
  };

  return renderImplictSurfaceColorBatch(parent, name, batchFunc, batchFuncColor, opts);
}

template <class Func, class FuncColor>
ColorRenderImage* renderImplictSurfaceColorBatch(std::string name, Func&& func, FuncColor&& funcColor,
                                                 ImplictRenderOpts opts) {}

} // namespace polyscope

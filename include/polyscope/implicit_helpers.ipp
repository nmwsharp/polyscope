// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once


#include "polyscope/camera_view.h"
#include "polyscope/floating_quantity_structure.h"
#include "polyscope/implicit_helpers.h"
#include "polyscope/messages.h"
#include "polyscope/view.h"

#include <tuple>
#include <vector>

namespace polyscope {

template <class S>
void resolveImplicitRenderOpts(QuantityStructure<S>* parent, ImplicitRenderOpts& opts) {

  // see comment in the ImplicitRenderOpts struct for the logic that this function implements

  // Case where camera params are explicitly given
  if (opts.cameraParameters.isValid()) {
    if (opts.dimX < 0 || opts.dimY < 0) {
      exception("if using explicit camera parameters, you must set render image resolution");
    }

    return;
  }

  // Case where we render from the current view
  if (std::is_same<S, CameraView>::value && parent != nullptr) {

    CameraView* parentCamera = dynamic_cast<CameraView*>(parent); // sorry
    opts.cameraParameters = parentCamera->getCameraParameters();

    if (opts.dimX < 0 || opts.dimY < 0) {
      exception("when rendering with camera parameters from a camera view, you must set render image resolution");
    }

    return;
  }


  // Case where we render from the current view
  if (std::is_same<S, FloatingQuantityStructure>::value && parent != nullptr) {

    if (view::projectionMode != ProjectionMode::Perspective) {
      // TODO to support orthographic, need to add view functions to get ray origins
      exception("implicit surface rendering from view only supports perspective projection");
    }

    opts.cameraParameters = view::getCameraParametersForCurrentView();
    opts.dimX = view::bufferWidth / opts.subsampleFactor;
    opts.dimY = view::bufferHeight / opts.subsampleFactor;

    return;
  }


  // Else: error, one of the other cases should have happened
  exception("implicit render opts must either specify camera parameters, render from a camera view, or add to the "
            "global floating structure to use the current view");
}

template <class Func>
std::tuple<std::vector<float>, std::vector<glm::vec3>, std::vector<glm::vec3>>
renderImplicitSurfaceTracer(Func&& func, ImplicitRenderMode mode, ImplicitRenderOpts opts) {

  // Read out option values
  const float missDist = opts.missDist.asAbsolute();
  const float hitDist = opts.hitDist.asAbsolute();
  const float stepFactor = opts.stepFactor;          // used for sphere march only
  const float stepSize = opts.stepSize.asAbsolute(); // used for fixed step only
  const size_t nMaxSteps = opts.nMaxSteps;
  const float normalSampleEps = opts.normalSampleEps;
  const int subsampleFactor = opts.subsampleFactor;


  CameraParameters& params = opts.cameraParameters;
  glm::vec3 cameraLoc = params.getPosition();
  glm::mat4x4 viewMat = params.getViewMat();
  size_t dimX = opts.dimX;
  size_t dimY = opts.dimY;
  size_t nPix = dimX * dimY;

  // Generate rays corresponding to each pixel
  // (this is a working set which will be shrunk as computation proceeds)
  std::vector<glm::vec3> rayRoots(nPix);
  std::vector<size_t> rayInds(nPix); // index of the ray
  for (size_t iY = 0; iY < dimY; iY++) {
    for (size_t iX = 0; iX < dimX; iX++) {
      size_t ind = iY * dimX + iX;
      rayRoots[ind] = cameraLoc;
      rayInds[ind] = ind;
    }
  }
  std::vector<glm::vec3> rayDirs = params.generateCameraRays(dimX, dimY, ImageOrigin::UpperLeft);

  // Sample the first value at each ray (to check for sign changes)
  std::vector<float> currVals(nPix);
  func(&rayRoots.front().x, &currVals.front(), rayRoots.size());

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
  for (size_t iStep = 0; (iStep < nMaxSteps) && (iFinished < nPix); iStep++) {

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
        float rayStepSize = -1.;
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
    currVals.resize(iPack);

    // Evaluate the remaining rays
    if (iPack > 0) {
      func(&currPos.front().x, &currVals.front(), currPos.size());
    }
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
  currVals.resize(nPix);
  for (size_t iV = 0; iV < 4; iV++) {
    glm::vec3 vertVec = tetVerts[iV];

    // Set up the evaluation points for each pixel
    for (size_t iP = 0; iP < nPix; iP++) {
      float f = rayDepthOut[iP] * normalSampleEps;
      currPos[iP] = rayPosOut[iP] + f * vertVec;
    }

    // Evaluate the function at each sample point
    func(&currPos.front().x, &currVals.front(), currPos.size());

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


  return std::tuple<std::vector<float>, std::vector<glm::vec3>, std::vector<glm::vec3>>{rayDepthOut, rayPosOut,
                                                                                        normalOut};
}

// =======================================================
// === Depth/geometry/shape only render functions
// =======================================================

template <class Func>
DepthRenderImageQuantity* renderImplicitSurface(std::string name, Func&& func, ImplicitRenderMode mode,
                                                ImplicitRenderOpts opts) {

  return renderImplicitSurface(getGlobalFloatingQuantityStructure(), name, func, mode, opts);
}

template <class Func>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(std::string name, Func&& func, ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts) {
  return renderImplicitSurfaceBatch(getGlobalFloatingQuantityStructure(), name, func, mode, opts);
}

template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurface(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                ImplicitRenderMode mode, ImplicitRenderOpts opts) {

  // Bootstrap on the batch version
  auto batchFunc = [&](const float* pos_ptr, float* result_ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
      glm::vec3 pos{
          pos_ptr[3 * i + 0],
          pos_ptr[3 * i + 1],
          pos_ptr[3 * i + 2],
      };
      result_ptr[i] = static_cast<float>(func(pos));
    }
  };

  return renderImplicitSurfaceBatch(parent, name, batchFunc, mode, opts);
}


template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     ImplicitRenderMode mode, ImplicitRenderOpts opts) {

  resolveImplicitRenderOpts(parent, opts);

  // Call the function which does all the hard work
  std::vector<float> rayDepthOut;
  std::vector<glm::vec3> rayPosOut;
  std::vector<glm::vec3> normalOut;
  std::tie(rayDepthOut, rayPosOut, normalOut) = renderImplicitSurfaceTracer(func, mode, opts);

  // TODO check if there is an existing quantity of the same type/size to replace, and if so re-fill its buffers
  // rather than creating a whole new one

  // here, we bypass the conversion adaptor since we have explicitly filled matching types
  return parent->addDepthRenderImageQuantityImpl(name, opts.dimX, opts.dimY, rayDepthOut, normalOut,
                                                 ImageOrigin::UpperLeft);
}

// =======================================================
// === Colored surface render functions
// =======================================================


template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColor(std::string name, Func&& func, FuncColor&& funcColor,
                                                     ImplicitRenderMode mode, ImplicitRenderOpts opts) {
  return renderImplicitSurfaceColor(getGlobalFloatingQuantityStructure(), name, func, funcColor, mode, opts);
}

template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(std::string name, Func&& func, FuncColor&& funcColor,
                                                          ImplicitRenderMode mode, ImplicitRenderOpts opts) {
  return renderImplicitSurfaceColorBatch(getGlobalFloatingQuantityStructure(), name, func, funcColor, mode, opts);
}


template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColor(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     FuncColor&& funcColor, ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts) {

  // Bootstrap on the batch version
  auto batchFunc = [&](const float* pos_ptr, float* result_ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
      glm::vec3 pos{
          pos_ptr[3 * i + 0],
          pos_ptr[3 * i + 1],
          pos_ptr[3 * i + 2],
      };
      result_ptr[i] = static_cast<float>(func(pos));
    }
  };

  auto batchFuncColor = [&](const float* pos_ptr, float* result_ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
      glm::vec3 pos{
          pos_ptr[3 * i + 0],
          pos_ptr[3 * i + 1],
          pos_ptr[3 * i + 2],
      };

      glm::vec3 color = funcColor(pos);

      result_ptr[3 * i + 0] = color.x;
      result_ptr[3 * i + 1] = color.y;
      result_ptr[3 * i + 2] = color.z;
    }
  };

  return renderImplicitSurfaceColorBatch(parent, name, batchFunc, batchFuncColor, mode, opts);
}


template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                          FuncColor&& funcColor, ImplicitRenderMode mode,
                                                          ImplicitRenderOpts opts) {

  resolveImplicitRenderOpts(parent, opts);

  // Call the function which does all the hard work
  std::vector<float> rayDepthOut;
  std::vector<glm::vec3> rayPosOut;
  std::vector<glm::vec3> normalOut;
  std::tie(rayDepthOut, rayPosOut, normalOut) = renderImplicitSurfaceTracer(func, mode, opts);

  // Batch evaluate the color function
  std::vector<glm::vec3> colorOut(rayPosOut.size());
  funcColor(&rayPosOut.front().x, &colorOut.front().x, rayPosOut.size());

  // Set colors for miss rays to 0
  for (size_t iP = 0; iP < rayPosOut.size(); iP++) {
    if (rayDepthOut[iP] == std::numeric_limits<float>::infinity()) {
      colorOut[iP] = glm::vec3{0.f, 0.f, 0.f};
    }
  }


  // TODO check if there is an existing quantity of the same type/size to replace, and if so re-fill its buffers
  // rather than creating a whole new one

  // here, we bypass the conversion adaptor since we have explicitly filled matching types
  return parent->addColorRenderImageQuantityImpl(name, opts.dimX, opts.dimY, rayDepthOut, normalOut, colorOut,
                                                 ImageOrigin::UpperLeft);
}


// =======================================================
// === Scalar surface render functions
// =======================================================
//
template <class Func, class FuncScalar>
ScalarRenderImageQuantity* renderImplicitSurfaceScalar(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                       ImplicitRenderMode mode, ImplicitRenderOpts opts,
                                                       DataType dataType) {
  return renderImplicitSurfaceScalar(getGlobalFloatingQuantityStructure(), name, func, funcScalar, mode, opts,
                                     dataType);
}

template <class Func, class FuncScalar>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                            ImplicitRenderMode mode, ImplicitRenderOpts opts,
                                                            DataType dataType) {

  return renderImplicitSurfaceScalarBatch(getGlobalFloatingQuantityStructure(), name, func, funcScalar, mode, opts,
                                          dataType);
}

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity* renderImplicitSurfaceScalar(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                       FuncScalar&& funcScalar, ImplicitRenderMode mode,
                                                       ImplicitRenderOpts opts, DataType dataType) {

  // Bootstrap on the batch version
  auto batchFunc = [&](const float* pos_ptr, float* result_ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
      glm::vec3 pos{
          pos_ptr[3 * i + 0],
          pos_ptr[3 * i + 1],
          pos_ptr[3 * i + 2],
      };
      result_ptr[i] = static_cast<float>(func(pos));
    }
  };

  auto batchFuncScalar = [&](const float* pos_ptr, float* result_ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
      glm::vec3 pos{
          pos_ptr[3 * i + 0],
          pos_ptr[3 * i + 1],
          pos_ptr[3 * i + 2],
      };
      result_ptr[i] = static_cast<float>(funcScalar(pos));
    }
  };

  return renderImplicitSurfaceScalarBatch(parent, name, batchFunc, batchFuncScalar, mode, opts, dataType);
}

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                            FuncScalar&& funcScalar, ImplicitRenderMode mode,
                                                            ImplicitRenderOpts opts, DataType dataType) {

  resolveImplicitRenderOpts(parent, opts);

  // Call the function which does all the hard work
  std::vector<float> rayDepthOut;
  std::vector<glm::vec3> rayPosOut;
  std::vector<glm::vec3> normalOut;
  std::tie(rayDepthOut, rayPosOut, normalOut) = renderImplicitSurfaceTracer(func, mode, opts);

  // Batch evaluate the color function
  std::vector<float> scalarOut(rayPosOut.size());
  funcScalar(&rayPosOut.front().x, &scalarOut.front(), rayPosOut.size());

  // Set scalars for miss rays to NaN
  const float nan = std::numeric_limits<float>::quiet_NaN();
  for (size_t iP = 0; iP < rayPosOut.size(); iP++) {
    if (rayDepthOut[iP] == std::numeric_limits<float>::infinity()) {
      scalarOut[iP] = nan;
    }
  }


  // TODO check if there is an existing quantity of the same type/size to replace, and if so re-fill its buffers
  // rather than creating a whole new one

  // here, we bypass the conversion adaptor since we have explicitly filled matching types
  std::vector<double> scalarOutD(rayPosOut.size());
  for (size_t i = 0; i < scalarOut.size(); i++) {
    scalarOutD[i] = scalarOut[i];
  }
  return parent->addScalarRenderImageQuantityImpl(name, opts.dimX, opts.dimY, rayDepthOut, normalOut, scalarOutD,
                                                  ImageOrigin::UpperLeft, dataType);
}


} // namespace polyscope

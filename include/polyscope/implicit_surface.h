// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/depth_render_image.h"
#include "polyscope/polyscope.h"
#include "polyscope/scaled_value.h"
#include "polyscope/structure.h"

#include <string>
#include <vector>

namespace polyscope {

// A collection of functions for rendering implicit surfaces

struct ImplictRenderOpts {
  ScaledValue<float> missDist = ScaledValue<float>::relative(20.);
  ScaledValue<float> hitDist = ScaledValue<float>::relative(1e-4);
  float stepFactor = 0.95; // used for sphere marching
  float normalSampleEps = 1e-3;
  ScaledValue<float> stepSize = ScaledValue<float>::relative(1e-2); // used for fixed-size stepping
  size_t nMaxSteps = 512;
};

// Renders an implicit surface via sphere marching rays from the current Polyscope camera view.
// The `func` argument is your implicit function, which takes a simple input `glm::vec3` in world-space coordinates,
// returns the value of the implicit function. For this version, the implicit function MUST be a "signed distance
// function", i.e. function is positive outside the surface, negative inside the surface, and the magnitude gives the
// distance to the surface (or technically, an upper bound on that distance). The "fixed step" version below handles
// more general implicit functions
template <class Func, class S>
DepthRenderImage* renderImplictSurfaceSphereMarch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                  ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func>
DepthRenderImage* renderImplictSurfaceSphereMarch(std::string name, Func&& func,
                                                  ImplictRenderOpts opts = ImplictRenderOpts()) {
  return renderImplictSurfaceSphereMarch(getGlobalFloatingQuantityStructure(), name, func, opts);
}
template <class Func, class S>
DepthRenderImage* renderImplictSurfaceSphereMarchBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                       ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func>
DepthRenderImage* renderImplictSurfaceSphereMarchBatch(std::string name, Func&& func,
                                                       ImplictRenderOpts opts = ImplictRenderOpts()) {
  return renderImplictSurfaceSphereMarchBatch(getGlobalFloatingQuantityStructure(), name, func, opts);
}

} // namespace polyscope

#include "polyscope/implicit_surface.ipp"

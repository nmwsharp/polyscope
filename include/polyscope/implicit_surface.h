// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/color_render_image.h"
#include "polyscope/depth_render_image.h"
#include "polyscope/polyscope.h"
#include "polyscope/scalar_render_image.h"
#include "polyscope/scaled_value.h"
#include "polyscope/structure.h"

#include <string>
#include <vector>

namespace polyscope {

// A collection of functions for rendering implicit surfaces

struct ImplictRenderOpts {
  ImplicitRenderMode mode = ImplicitRenderMode::SphereMarch;
  ScaledValue<float> missDist = ScaledValue<float>::relative(20.);
  ScaledValue<float> hitDist = ScaledValue<float>::relative(1e-4);
  float stepFactor = 0.99; // used for sphere marching
  float normalSampleEps = 1e-3;
  ScaledValue<float> stepSize = ScaledValue<float>::relative(1e-2); // used for fixed-size stepping
  size_t nMaxSteps = 1024;
  int subsampleFactor = 1;
};

// =======================================================
// === Depth/geometry/shape only render functions
// =======================================================

// Renders an implicit surface via sphere marching rays from the current Polyscope camera view.
// The `func` argument is your implicit function, which takes a simple input `glm::vec3` in world-space coordinates,
// returns the value of the implicit function. For this version, the implicit function MUST be a "signed distance
// function", i.e. function is positive outside the surface, negative inside the surface, and the magnitude gives the
// distance to the surface (or technically, an upper bound on that distance). The "fixed step" version below handles
// more general implicit functions
template <class Func, class S>
DepthRenderImage* renderImplictSurface(QuantityStructure<S>* parent, std::string name, Func&& func,
                                       ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func>
DepthRenderImage* renderImplictSurface(std::string name, Func&& func, ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func, class S>
DepthRenderImage* renderImplictSurfaceBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                            ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func>
DepthRenderImage* renderImplictSurfaceBatch(std::string name, Func&& func,
                                            ImplictRenderOpts opts = ImplictRenderOpts());

// =======================================================
// === Colored surface render functions
// =======================================================

// Like the implicit surface renderers above, but additionally take a color

template <class Func, class FuncColor, class S>
ColorRenderImage* renderImplictSurfaceColor(QuantityStructure<S>* parent, std::string name, Func&& func,
                                            FuncColor&& funcColor, ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func, class FuncColor>
ColorRenderImage* renderImplictSurfaceColor(std::string name, Func&& func, FuncColor&& funcColor,
                                            ImplictRenderOpts opts = ImplictRenderOpts());

template <class Func, class FuncColor, class S>
ColorRenderImage* renderImplictSurfaceColorBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                 FuncColor&& funcColor, ImplictRenderOpts opts = ImplictRenderOpts());
template <class Func, class FuncColor>
ColorRenderImage* renderImplictSurfaceColorBatch(std::string name, Func&& func, FuncColor&& funcColor,
                                                 ImplictRenderOpts opts = ImplictRenderOpts());

// =======================================================
// === Scalar surface render functions
// =======================================================

// Like the implicit surface renderers above, but additionally take a scalar and colormap it, etc

template <class Func, class FuncScalar, class S>
ScalarRenderImage* renderImplictSurfaceScalar(QuantityStructure<S>* parent, std::string name, Func&& func,
                                              FuncScalar&& funcScalar, ImplictRenderOpts opts = ImplictRenderOpts(),
                                              DataType dataType = DataType::STANDARD);
template <class Func, class FuncScalar>
ScalarRenderImage* renderImplictSurfaceScalar(std::string name, Func&& func, FuncScalar&& funcScalar,
                                              ImplictRenderOpts opts = ImplictRenderOpts(),
                                              DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar, class S>
ScalarRenderImage*
renderImplictSurfaceScalarBatch(QuantityStructure<S>* parent, std::string name, Func&& func, FuncScalar&& funcScalar,
                                ImplictRenderOpts opts = ImplictRenderOpts(), DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar>
ScalarRenderImage* renderImplictSurfaceScalarBatch(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                   ImplictRenderOpts opts = ImplictRenderOpts(),
                                                   DataType dataType = DataType::STANDARD);


} // namespace polyscope

#include "polyscope/implicit_surface.ipp"

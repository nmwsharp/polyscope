// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"

#include "polyscope/color_render_image_quantity.h"
#include "polyscope/depth_render_image_quantity.h"
#include "polyscope/scalar_render_image_quantity.h"
#include "polyscope/scaled_value.h"
#include "polyscope/structure.h"

#include <string>
#include <vector>

namespace polyscope {

// A collection of functions for rendering implicit surfaces

struct ImplicitRenderOpts {
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
// returns the value of the implicit function. For the "batch" variants, your function must take a
// std::vector<glm::vec3>, and produce a std::vector<float>.
//
// If using ImplicitRenderOpts::SphereMarch, the implicit function MUST be a "signed distance
// function", i.e. function is positive outside the surface, negative inside the surface, and the magnitude gives the
// distance to the surface (or technically, an upper bound on that distance). Alternately, ImplicitRenderOpts::FixedStep
// handles more general implicit functions. See the options struct for other options.
template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurface(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func>
DepthRenderImageQuantity* renderImplicitSurface(std::string name, Func&& func,
                                                ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(std::string name, Func&& func,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());

// =======================================================
// === Colored surface render functions
// =======================================================

// Like the implicit surface renderers above, but additionally take a color

template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColor(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     FuncColor&& funcColor,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColor(std::string name, Func&& func, FuncColor&& funcColor,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());

template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                          FuncColor&& funcColor,
                                                          ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(std::string name, Func&& func, FuncColor&& funcColor,
                                                          ImplicitRenderOpts opts = ImplicitRenderOpts());

// =======================================================
// === Scalar surface render functions
// =======================================================

// Like the implicit surface renderers above, but additionally take a scalar and colormap it, etc

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity*
renderImplicitSurfaceScalar(QuantityStructure<S>* parent, std::string name, Func&& func, FuncScalar&& funcScalar,
                            ImplicitRenderOpts opts = ImplicitRenderOpts(), DataType dataType = DataType::STANDARD);
template <class Func, class FuncScalar>
ScalarRenderImageQuantity* renderImplicitSurfaceScalar(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                       ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                       DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                            FuncScalar&& funcScalar,
                                                            ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                            DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                            ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                            DataType dataType = DataType::STANDARD);


} // namespace polyscope

#include "polyscope/implicit_surface.ipp"

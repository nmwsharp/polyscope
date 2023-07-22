// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"

#include "polyscope/color_render_image_quantity.h"
#include "polyscope/depth_render_image_quantity.h"
#include "polyscope/scalar_render_image_quantity.h"
#include "polyscope/scaled_value.h"
#include "polyscope/structure.h"
#include "polyscope/utilities.h"

#include <string>
#include <vector>

namespace polyscope {

// A collection of helper functions for generating visualizations of implicitly-defined data (that is, where you have a
// function that you can evaluate at f(x,y,z) to get back a scalar, color, etc.

// =======================================================
// === Render implicit surfaces
// =======================================================

struct ImplicitRenderOpts {

  // = Options for how the image is defined

  // (1) If camera parameters & resolution are passed, in these options, they will always be respected.
  //
  // (2) Otherwise, if the parent structure is null (or the global floating struct), we will render from the current
  // polyscope camera view, and take the resolution etc from that.
  //
  // (3) Otherwise, if the parent structure is a camera view, we will take the camera parameters from that, but the
  // dimensions must be specified.
  //
  // (4) Otherwise, if the parent structure is a structure other than the camera view, the parameters should have been
  // explicitly specified as in (1), and an error will be thrown.

  // The camera parameters to use.
  // If left as the default uninitialized camera, it will be overwritten according to the policies above.
  CameraParameters cameraParameters = CameraParameters::createInvalid();

  // The dimensions at which to render the image.
  // These normally must be set explicitly, unless we are rendering from the current view as specified above.
  int32_t dimX = -1;
  int32_t dimY = -1;

  // If dimX and dimY are being set automatically, downscale them by this factor (e.g. subsampleFactor=2 means use
  // dimX/2 and dimY/2)
  int subsampleFactor = 1;

  // = Options for the rendering computation itself

  // How far the ray must go before it is abandoned as a miss
  ScaledValue<float> missDist = ScaledValue<float>::relative(20.);

  // How small the the value of the implicit function must be to be considered a hit
  ScaledValue<float> hitDist = ScaledValue<float>::relative(1e-4);

  // For mode == SphereMarch, a small tolerance factor applied to step sizes
  float stepFactor = 0.99;

  // Used to estimate normals via finite differences, also used relative value times the hit distance.
  float normalSampleEps = 1e-3;

  // The size of the steps used for mode == FixedStep
  ScaledValue<float> stepSize = ScaledValue<float>::relative(1e-2);

  // The maximum number of steps to take
  size_t nMaxSteps = 1024;
};

// Populate the custom-filled entries of opts according to the policy above.
template <class S>
void resolveImplicitRenderOpts(QuantityStructure<S>* parent, ImplicitRenderOpts& opts);

// === Depth/geometry/shape only render functions

// Renders an implicit surface by shooting a ray for each pixel and querying the implicit function along the ray.
// Supports sphere marching (for implicit functions which are SDFs), and fixed step marching (for general implicit
// functions). Renering can be performed from the current GUI viewport, from a specified set of camera parameters, or
// from a give CameraView object. See docs for the `opts` parameter for details.
//
// The `func` argument is your implicit function, which takes a simple input `glm::vec3` in world-space coordinates,
// returns the value of the implicit function.
//
// For the "batch" variants, your function must have the signature
// void(float* in_pos_ptr, float* out_val_ptr, size_t N). The first arg is a length-3N array of positions for queries,
// and the second is a length-N (already-allocated) array of values which you should write to. The color and scalar
// variants below are similar, except that for color the output array has length 3N.
//
// If using ImplicitRenderMode::SphereMarch, the implicit function MUST be a "signed distance
// function", i.e. function is positive outside the surface, negative inside the surface, and the magnitude gives the
// distance to the surface (or technically, an upper bound on that distance). Alternately, ImplicitRenderMode::FixedStep
// handles more general implicit functions. See the options struct for other options.

template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurface(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                ImplicitRenderMode mode,
                                                ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func>
DepthRenderImageQuantity* renderImplicitSurface(std::string name, Func&& func, ImplicitRenderMode mode,
                                                ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class S>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func>
DepthRenderImageQuantity* renderImplicitSurfaceBatch(std::string name, Func&& func, ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());

// === Colored surface render functions

// Like the implicit surface renderers above, but additionally take a color

template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColor(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                     FuncColor&& funcColor, ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColor(std::string name, Func&& func, FuncColor&& funcColor,
                                                     ImplicitRenderMode mode,
                                                     ImplicitRenderOpts opts = ImplicitRenderOpts());

template <class Func, class FuncColor, class S>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                          FuncColor&& funcColor, ImplicitRenderMode mode,
                                                          ImplicitRenderOpts opts = ImplicitRenderOpts());
template <class Func, class FuncColor>
ColorRenderImageQuantity* renderImplicitSurfaceColorBatch(std::string name, Func&& func, FuncColor&& funcColor,
                                                          ImplicitRenderMode mode,
                                                          ImplicitRenderOpts opts = ImplicitRenderOpts());

// === Scalar surface render functions

// Like the implicit surface renderers above, but additionally take a scalar and colormap it, etc

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity* renderImplicitSurfaceScalar(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                       FuncScalar&& funcScalar, ImplicitRenderMode mode,
                                                       ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                       DataType dataType = DataType::STANDARD);
template <class Func, class FuncScalar>
ScalarRenderImageQuantity*
renderImplicitSurfaceScalar(std::string name, Func&& func, FuncScalar&& funcScalar, ImplicitRenderMode mode,
                            ImplicitRenderOpts opts = ImplicitRenderOpts(), DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar, class S>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(QuantityStructure<S>* parent, std::string name, Func&& func,
                                                            FuncScalar&& funcScalar, ImplicitRenderMode mode,
                                                            ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                            DataType dataType = DataType::STANDARD);

template <class Func, class FuncScalar>
ScalarRenderImageQuantity* renderImplicitSurfaceScalarBatch(std::string name, Func&& func, FuncScalar&& funcScalar,
                                                            ImplicitRenderMode mode,
                                                            ImplicitRenderOpts opts = ImplicitRenderOpts(),
                                                            DataType dataType = DataType::STANDARD);


} // namespace polyscope

#include "polyscope/implicit_helpers.ipp"

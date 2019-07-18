// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once


namespace polyscope {
namespace gl {

// Draw the ground plane to the active framebuffer
void drawGroundPlane();

void buildGroundPlaneGui();

// Free any resources related to the ground plane. Does nothing if no such resources have been allocated.
void deleteGroundPlaneResources();

// Options

// Should the ground plane be shown? Default: true
extern bool groundPlaneEnabled;

// How far should the ground plane be from the bottom of the scene? Measured as a multiple of the vertical bounding box
// of the scene.
// Default: 0.0
extern float groundPlaneHeightFactor;

} // namespace gl
} // namespace polyscope

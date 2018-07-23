#pragma once


namespace polyscope {

// Draw the ground plane to the active framebuffer
void drawGroundPlane();

// Free any resources related to the ground plane. Does nothing if no such resources have been allocated.
void deleteGroundPlaneResources();
};

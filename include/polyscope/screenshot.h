// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/polyscope.h"

namespace polyscope {


// Take screenshots of the current view
void screenshot(std::string filename, bool transparentBG = true);
void screenshot(bool transparentBG = true);
void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels);
void resetScreenshotIndex();


namespace state {

// The current screenshot index for automatically numbered screenshots
extern size_t screenshotInd;

} // namespace state
} // namespace polyscope

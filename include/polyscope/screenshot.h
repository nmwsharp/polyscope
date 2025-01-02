// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

namespace polyscope {


// Take a screenshot from the current view and write to file
void screenshot(bool transparentBG = true); // automatic file names like `screenshot_000000.png`
void screenshot(std::string filename, bool transparentBG = true);
void screenshot(const char* filename); // this is needed because annoyingly overload resolution prefers the bool version
void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels); // helper
void resetScreenshotIndex();

// Take a screenshot from the current view and return it as a buffer
// the dimensions are view::bufferWidth and view::bufferHeight , with entries RGBA at 1 byte each.
std::vector<unsigned char> screenshotToBuffer(bool transparentBG = true);

namespace state {

// The current screenshot index for automatically numbered screenshots
extern size_t screenshotInd;

} // namespace state
} // namespace polyscope

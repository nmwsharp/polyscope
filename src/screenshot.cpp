// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/screenshot.h"

#include "polyscope/polyscope.h"

#include "stb_image_write.h"

#include <algorithm>
#include <string>

namespace polyscope {

namespace state {

// Storage for the screenshot index
size_t screenshotInd = 0;

} // namespace state

// Helper functions
namespace {

bool hasExtension(std::string str, std::string ext) {

  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (str.length() >= ext.length()) {
    return (0 == str.compare(str.length() - ext.length(), ext.length(), ext));
  } else {
    return false;
  }
}

} // namespace


void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels) {

  // Auto-detect filename
  if (hasExtension(name, ".png")) {
    stbi_write_png(name.c_str(), w, h, channels, buffer, channels * w);
    //} else if(hasExtension(name, ".jpg") || hasExtension(name, "jpeg")) {
    // stbi_write_jgp(name.c_str(), w, h, channels, buffer);
  } else if (hasExtension(name, ".tga")) {
    stbi_write_tga(name.c_str(), w, h, channels, buffer);
  } else if (hasExtension(name, ".bmp")) {
    stbi_write_bmp(name.c_str(), w, h, channels, buffer);
  } else {
    // Fall back on png
    stbi_write_png(name.c_str(), w, h, channels, buffer, channels * w);
  }
}

void screenshot(std::string filename, bool transparentBG) {

  // Make sure we render first
  requestRedraw();
  draw(false);

  // these _should_ always be accurate
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->readDisplayBuffer();

  // Just flip
  if (transparentBG) {

    size_t flipBuffSize = w * h * 4;
    unsigned char* flipBuff = new unsigned char[flipBuffSize];
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        int flipInd = i + (h - j - 1) * w;
        flipBuff[4 * flipInd + 0] = buff[4 * ind + 0];
        flipBuff[4 * flipInd + 1] = buff[4 * ind + 1];
        flipBuff[4 * flipInd + 2] = buff[4 * ind + 2];
        flipBuff[4 * flipInd + 3] = buff[4 * ind + 3];
      }
    }

    // Save to file
    saveImage(filename, flipBuff, w, h, 4);

    delete[] flipBuff;
  }
  // Strip alpha channel and flip
  else {

    size_t noAlphaBuffSize = w * h * 3;
    unsigned char* noAlphaBuff = new unsigned char[noAlphaBuffSize];
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        int flipInd = i + (h - j - 1) * w;
        noAlphaBuff[3 * flipInd + 0] = buff[4 * ind + 0];
        noAlphaBuff[3 * flipInd + 1] = buff[4 * ind + 1];
        noAlphaBuff[3 * flipInd + 2] = buff[4 * ind + 2];
      }
    }

    // Save to file
    saveImage(filename, noAlphaBuff, w, h, 3);

    delete[] noAlphaBuff;
  }

}

void screenshot(bool transparentBG) {

  char buff[50];
  snprintf(buff, 50, "screenshot_%06zu.tga", state::screenshotInd);
  std::string defaultName(buff);

  screenshot(defaultName, transparentBG);

  state::screenshotInd++;
}

void resetScreenshotIndex() { state::screenshotInd = 0; }

} // namespace polyscope

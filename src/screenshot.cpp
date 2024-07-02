// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/screenshot.h"

#include "polyscope/polyscope.h"

#include "stb_image_write.h"

#include <algorithm>
#include <string>
#include <stdio.h>

namespace polyscope {

namespace state {

// Storage for the screenshot index
size_t screenshotInd = 0;

// Storage for the rasterizeTetra index
size_t rasterizeTetraInd = 0;

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

/* Opens a FILE pipe to FFmpeg so that we can write to .mp4 video file.
 * 
 * @param name: The name of the .mp4 file, such as "teapot.mp4".
 * @return FILE* file descriptor.
 */
FILE* openVideoFile(std::string filename, int fps) {
  // Create the FFmpeg command
  std::string cmd = "ffmpeg -r " + std::to_string(fps) + " "
                    "-f rawvideo "    // expect raw video input
                    "-pix_fmt rgba "  // expect RGBA input
                    "-s 2560x1440 "   // video dimensions (default polyscope window size)
                    "-i - "           // FFMpeg will read input from stdin
                    "-threads 0 "     // use optimal number of threads
                    "-preset fast "   // use fast encoding preset
                    "-y "             // overwrite output file without asking
                    "-pix_fmt yuv420p " // convert the pixel format to YUV420p for output
                    "-crf 21 "        // set constant rate factor 
                    "-vf vflip "      // buffer is from OpenGL, so need to vertically flip
                    + filename;

  // Open a pipe to FFmpeg
  FILE* ffmpeg = popen(cmd.c_str(), "w");
  return ffmpeg;
}

/* Closes a FILE pipe to FFmpeg.
 *
 * @param fd: This file descriptor should have been obtained from a prior call
 *            to openVideoFile.
 * @return: -1 if closeVideoFile fails, not -1 if successful.
 */
int closeVideoFile(FILE* fd) {
  if (!fd) {
    return -1;
  }
  return pclose(fd);
}


void saveImage(std::string filename, unsigned char* buffer, int w, int h, int channels) {

  // our buffers are from openGL, so they are flipped
  stbi_flip_vertically_on_write(1);
  stbi_write_png_compression_level = 0;

  // Auto-detect filename
  if (hasExtension(filename, ".png")) {
    stbi_write_png(filename.c_str(), w, h, channels, buffer, channels * w);
  } else if (hasExtension(filename, ".jpg") || hasExtension(filename, "jpeg")) {
    stbi_write_jpg(filename.c_str(), w, h, channels, buffer, 100);

    // TGA seems to display different on different machines: our fault or theirs?
    // Both BMP and TGA need alpha channel stripped? bmp doesn't seem to work even with this
    /*
    } else if (hasExtension(name, ".tga")) {
     stbi_write_tga(name.c_str(), w, h, channels, buffer);
    } else if (hasExtension(name, ".bmp")) {
     stbi_write_bmp(name.c_str(), w, h, channels, buffer);
    */

  } else {
    // Fall back on png
    stbi_write_png(filename.c_str(), w, h, channels, buffer, channels * w);
  }
}


/* Write a single video frame to .mp4 video file.
 *
 * @param fd: This file descriptor must have been obtained through a prior call
 *            to openVideoFile().
 * @param transparentBG: Whether or not transparency is enabled.
 * @return: -1 if writeVideoFrame fails, 0 on success.
 */
int writeVideoFrame(FILE* fd, bool transparentBG) {
  if (!fd) {
    return -1;
  }

  render::engine->useAltDisplayBuffer = true;
  if (transparentBG) render::engine->lightCopy = true; // copy directly in to buffer without blending

  // Make sure we render first
  processLazyProperties();

  // Save the redraw requested bit and restore it below
  bool requestedAlready = redrawRequested();
  requestRedraw();

  draw(false, false);

  if (requestedAlready) {
    requestRedraw();
  }

  // These _should_ always be accurate
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->displayBufferAlt->readBuffer();

  // Set alpha to 1
  if (!transparentBG) {
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        buff[4 * ind + 3] = std::numeric_limits<unsigned char>::max();
      }
    }
  }

  // Write to the FFmpeg pipe
  size_t r = fwrite(&(buff.front()), sizeof(unsigned char) * w * h * 4, 1, fd);
  if (r != 1) { // fwrite failed to write the full buffer
    return -1;
  }

  return 0;
}


void screenshot(std::string filename, bool transparentBG) {

  render::engine->useAltDisplayBuffer = true;
  if (transparentBG) render::engine->lightCopy = true; // copy directly in to buffer without blending

  // == Make sure we render first
  processLazyProperties();

  // save the redraw requested bit and restore it below
  bool requestedAlready = redrawRequested();
  requestRedraw();

  draw(false, false);

  if (requestedAlready) {
    requestRedraw();
  }

  // these _should_ always be accurate
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->displayBufferAlt->readBuffer();

  // Set alpha to 1
  if (!transparentBG) {
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        buff[4 * ind + 3] = std::numeric_limits<unsigned char>::max();
      }
    }
  }

  // Save to file
  saveImage(filename, &(buff.front()), w, h, 4);

  render::engine->useAltDisplayBuffer = false;
  if (transparentBG) render::engine->lightCopy = false;
}

void screenshot(bool transparentBG) {

  char buff[50];
  snprintf(buff, 50, "screenshot_%06zu%s", state::screenshotInd, options::screenshotExtension.c_str());
  std::string defaultName(buff);

  // only pngs can be written with transparency
  if (!hasExtension(options::screenshotExtension, ".png")) {
    transparentBG = false;
  }

  screenshot(defaultName, transparentBG);

  state::screenshotInd++;
}

void resetScreenshotIndex() { state::screenshotInd = 0; }

std::vector<unsigned char> screenshotToBuffer(bool transparentBG) {

  render::engine->useAltDisplayBuffer = true;
  if (transparentBG) render::engine->lightCopy = true; // copy directly in to buffer without blending

  // == Make sure we render first
  processLazyProperties();

  // save the redraw requested bit and restore it below
  bool requestedAlready = redrawRequested();
  requestRedraw();

  draw(false, false);

  if (requestedAlready) {
    requestRedraw();
  }

  // these _should_ always be accurate
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->displayBufferAlt->readBuffer();

  // Set alpha to 1
  if (!transparentBG) {
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        buff[4 * ind + 3] = std::numeric_limits<unsigned char>::max();
      }
    }
  }

  render::engine->useAltDisplayBuffer = false;
  if (transparentBG) render::engine->lightCopy = false;

  return buff;
}


void rasterizeTetra(std::string filename) {
  render::engine->useAltDisplayBuffer = true;
  render::engine->lightCopy = true;

  processLazyProperties();

  bool requestedAlready = redrawRequested();
  requestRedraw();

  draw(false, false);

  if (requestedAlready) {
  requestRedraw();
  }

  // We will grab sceneBufferFinal, which contains scene colors before tone mapping.
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->sceneBufferFinal->readBuffer();

  // Save to file
  saveImage(filename, &(buff.front()), w, h, 4);

  render::engine->useAltDisplayBuffer = false;
  render::engine->lightCopy = false;
} 

void rasterizeTetra() {
  char buff[50];
  snprintf(buff, 50, "tetra_%06zu%s", state::rasterizeTetraInd, options::screenshotExtension.c_str());
  std::string defaultName(buff);

  rasterizeTetra(defaultName);

  state::rasterizeTetraInd++;
}

} // namespace polyscope

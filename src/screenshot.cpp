// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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

// Helper to actually do the render pass and return the result in a buffer
std::vector<unsigned char> getRenderInBuffer(const ScreenshotOptions& options = {}) {
  checkInitialized();

  render::engine->useAltDisplayBuffer = true;
  if (options.transparentBackground) render::engine->lightCopy = true; // copy directly in to buffer without blending

  // == Make sure we render first
  processLazyProperties();

  // save the redraw requested bit and restore it below
  bool requestedAlready = redrawRequested();
  requestRedraw();

  // There's a ton of junk needed here to handle the includeUI case...
  // Create a new context and push it on to the stack
  // FIXME this solution doesn't really work, it forgets UI state like which nodes were open, scrolled setting, etc.
  // I'm not sure if it's possible to do this like we want in ImGui. The alternate solution would be to save the render
  // from the previous render pass, but I think that comes with other problems on the Polyscope side. I'm not sure what
  // the answer is.
  ImGuiContext* oldContext;
  ImGuiContext* newContext;
  ImPlotContext* oldPlotContext;
  ImPlotContext* newPlotContext;
  if (options.includeUI) {
    // WARNING: code duplicated here and in pushContext()
    oldContext = ImGui::GetCurrentContext();
    newContext = ImGui::CreateContext();
    oldPlotContext = ImPlot::GetCurrentContext();
    newPlotContext = ImPlot::CreateContext();
    ImGuiIO& oldIO = ImGui::GetIO(); // used to GLFW + OpenGL data to the new IO object
#ifdef IMGUI_HAS_DOCK
    ImGuiPlatformIO& oldPlatformIO = ImGui::GetPlatformIO();
#endif
    ImGui::SetCurrentContext(newContext);
    ImPlot::SetCurrentContext(newPlotContext);

#ifdef IMGUI_HAS_DOCK
    // Propagate GLFW window handle to new context
    ImGui::GetMainViewport()->PlatformHandle = oldPlatformIO.Viewports[0]->PlatformHandle;
#endif
    ImGui::GetIO().BackendPlatformUserData = oldIO.BackendPlatformUserData;
    ImGui::GetIO().BackendRendererUserData = oldIO.BackendRendererUserData;

    render::engine->configureImGui();

    // render a few times, to let imgui shake itself out
    for (int i = 0; i < 3; i++) {
      draw(options.includeUI, false);
    }
  }

  draw(options.includeUI, false);

  if (options.includeUI) {
    // WARNING: code duplicated here and in pushContext()
    // Workaround overzealous ImGui assertion before destroying any inner context
    // https://github.com/ocornut/imgui/pull/7175
    ImGui::SetCurrentContext(newContext);
    ImPlot::SetCurrentContext(newPlotContext);
    ImGui::GetIO().BackendPlatformUserData = nullptr;
    ImGui::GetIO().BackendRendererUserData = nullptr;

    ImPlot::DestroyContext(newPlotContext);
    ImGui::DestroyContext(newContext);

    ImGui::SetCurrentContext(oldContext);
    ImPlot::SetCurrentContext(oldPlotContext);
  }


  if (requestedAlready) {
    requestRedraw();
  }

  // these _should_ always be accurate
  int w = view::bufferWidth;
  int h = view::bufferHeight;
  std::vector<unsigned char> buff = render::engine->displayBufferAlt->readBuffer();

  // Set alpha to 1
  if (!options.transparentBackground) {
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        int ind = i + j * w;
        buff[4 * ind + 3] = std::numeric_limits<unsigned char>::max();
      }
    }
  }

  render::engine->useAltDisplayBuffer = false;
  if (options.transparentBackground) render::engine->lightCopy = false;

  return buff;
}


} // namespace


void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels) {
  checkInitialized();

  // our buffers are from openGL, so they are flipped
  stbi_flip_vertically_on_write(1);
  stbi_write_png_compression_level = 0;

  // Auto-detect filename
  if (hasExtension(name, ".png")) {
    stbi_write_png(name.c_str(), w, h, channels, buffer, channels * w);
  } else if (hasExtension(name, ".jpg") || hasExtension(name, "jpeg")) {
    stbi_write_jpg(name.c_str(), w, h, channels, buffer, 100);

    // TGA seems to display different on different machines: our fault or theirs?
    // Both BMP and TGA need alpha channel stripped? bmp doesn't seem to work even with this
    /*
    } else if (hasExtension(name, ".tga")) {
     stbi_write_tga(name.c_str(), w, h, channels, buffer);
    } else if (hasExtension(name, ".bmp")) {
     stbi_write_bmp(name.c_str(), w, h, channels, buffer);
    */

  } else {
    error("unrecognized file extension, should be one of '.png', '.jpg', '.jpeg'. Got filename: " + name);
  }
}

void screenshot(std::string filename, const ScreenshotOptions& options) {
  checkInitialized();
  ScreenshotOptions thisOptions = options; // we may modify it below

  // only pngs can be written with transparency
  if (!hasExtension(filename, ".png")) {
    thisOptions.transparentBackground = false;
  }

  std::vector<unsigned char> buff = getRenderInBuffer(thisOptions);
  int w = view::bufferWidth;
  int h = view::bufferHeight;

  // Save to file
  saveImage(filename, &(buff.front()), w, h, 4);
}

void screenshot(std::string filename, bool transparentBG) {
  ScreenshotOptions options;
  options.transparentBackground = transparentBG;
  screenshot(filename, options);
}

void screenshot(const ScreenshotOptions& options) {

  // construct the filename for the output
  char buff[50];
  snprintf(buff, 50, "screenshot_%06zu%s", state::screenshotInd, options::screenshotExtension.c_str());
  std::string defaultName(buff);

  screenshot(defaultName, options);

  state::screenshotInd++;
}

void screenshot(bool transparentBG) {
  ScreenshotOptions options;
  options.transparentBackground = transparentBG;
  screenshot(options);
}

void screenshot(const char* filename) { screenshot(std::string(filename), true); }

void resetScreenshotIndex() { state::screenshotInd = 0; }


std::vector<unsigned char> screenshotToBuffer(const ScreenshotOptions& options) { return getRenderInBuffer(options); }

std::vector<unsigned char> screenshotToBuffer(bool transparentBG) {
  ScreenshotOptions options;
  options.transparentBackground = transparentBG;
  return screenshotToBuffer(options);
}

} // namespace polyscope

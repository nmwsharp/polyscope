#include "polyscope/polyscope.h"

#include <algorithm>
#include <string>

#include "stb_image_write.h"


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

namespace polyscope {

void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels) {

  // Auto-detect filename
  if(hasExtension(name, ".png")) {
    stbi_write_png(name.c_str(), w, h, channels, buffer, channels * w);
  //} else if(hasExtension(name, ".jpg") || hasExtension(name, "jpeg")) {
    //stbi_write_jgp(name.c_str(), w, h, channels, buffer);
  } else if(hasExtension(name, ".tga")) {
    stbi_write_tga(name.c_str(), w, h, channels, buffer);
  } else if(hasExtension(name, ".bmp")) {
    stbi_write_bmp(name.c_str(), w, h, channels, buffer);
  } else {
    // Fall back on png
    stbi_write_png(name.c_str(), w, h, channels, buffer, channels * w);
  }
}

} // namespace polyscope

#include "polyscope/polyscope.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace polyscope {

void saveImage(std::string name, unsigned char* buffer, int w, int h, int channels) {
  stbi_write_png(name.c_str(), w, h, channels, buffer, channels*w);
}

}
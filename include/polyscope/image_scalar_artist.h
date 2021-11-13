// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/render/engine.h"
#include "polyscope/scalar_quantity.h"


#include <vector>


namespace polyscope {

template <typename QuantityT>
class ImageScalarArtist : public ScalarQuantity<QuantityT> {

public:
  // ImageScalarArtist(, DataType dataType);
  ImageScalarArtist(QuantityT& parentQ, std::string displayName, size_t dimX, size_t dimY,
                    const std::vector<double>& data, DataType dataType);

  // An alternate constructor which bypasses the float array and just reads directly from the texture. Limits will be
  // set arbitrarily. This is a bit of a hack, and mainly used for visualizing internal rendering buffers.
  // ImageScalarArtist(std::string name, std::shared_ptr<render::TextureBuffer>& texturebuffer, size_t dimX, size_t
  // dimY, DataType dataType = DataType::STANDARD);

  void renderSource();      // (re-)render the data to the internal texture
  void showFullscreen();  // render the image fullscreen
  void showInImGuiWindow(); // build a floating imgui window showing the texture
  void refresh();           // clear out and reinitialze

  const std::string displayName;
  const size_t dimX, dimY;
  const bool readFromTex = false; // hack to also support pulling directly from a texture


  // === Get/set visualization parameters


private:
  // UI internals
  std::shared_ptr<render::TextureBuffer> textureRaw, textureRendered;
  std::shared_ptr<render::FrameBuffer> framebuffer;
  std::shared_ptr<render::ShaderProgram> sourceProgram, fullscreenProgram;

  void prepare();
  void prepareSource();
  //void prepareFullscreen();
};

} // namespace polyscope

#include "polyscope/image_scalar_artist.ipp"

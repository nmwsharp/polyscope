// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/render/engine.h"
#include "polyscope/scalar_quantity.h"


#include <vector>


namespace polyscope {

// This class being templated on a quantity is an annoyance inherited from ScalarQuantity... would be nice to redesign
// to avoid it, but for now it's okay.
template <typename QuantityT>
class ImageScalarArtist : public ScalarQuantity<QuantityT> {

public:
  // ImageScalarArtist(, DataType dataType);
  ImageScalarArtist(QuantityT& parentQ, std::string displayName, size_t dimX, size_t dimY,
                    const std::vector<double>& data, ImageOrigin imageOrigin, DataType dataType);

  // An alternate constructor which bypasses the float array and just reads directly from the texture. Limits will be
  // set arbitrarily. This is a bit of a hack, and mainly used for visualizing internal rendering buffers.
  // ImageScalarArtist(std::string name, std::shared_ptr<render::TextureBuffer>& texturebuffer, size_t dimX, size_t
  // dimY, DataType dataType = DataType::STANDARD);

  void showFullscreen();    // render the image fullscreen
  void showInImGuiWindow(); // build a floating imgui window showing the texture, MUST call renderIntermediate() first

  void renderIntermediate(); // call this at draw time if you intend to call showInImGuiWindow() later


  // render to a rectangle in 3D
  // note that the magnitudes of upVec matters, it determines the size of the billboard in world space
  // the magnitude of rightVec is ignored and scaled to match the aspect ratio of the image
  void showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec);

  void refresh(); // clear out and reinitialze

  const std::string displayName;
  const size_t dimX, dimY;
  const bool readFromTex = false; // hack to also support pulling directly from a texture


  // === Get/set visualization parameters

  void setTransparency(float newVal);
  float getTransparency();

protected:
  PersistentValue<float> transparency;
  ImageOrigin imageOrigin;

private:
  // UI internals
  std::shared_ptr<render::TextureBuffer> textureRaw, textureIntermediateRendered;
  std::shared_ptr<render::ShaderProgram> fullscreenProgram, billboardProgram;
  std::shared_ptr<render::FrameBuffer> framebufferIntermediate;

  void prepareFullscreen();
  void prepareIntermediateRender();
  void prepareBillboard();
  void ensureRawTexturePopulated();
};

} // namespace polyscope

#include "polyscope/image_scalar_artist.ipp"

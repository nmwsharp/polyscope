// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <set>
#include <string>
#include <vector>

#include "polyscope/weak_handle.h"

namespace polyscope {

// This is a simple class which manages global state amongs Polyscope quanties which draw directly to the whole screen.
// It allows us to ensure only one is drawing to the screen at a time.


class FullscreenArtist : public virtual WeakReferrable {
public:
  FullscreenArtist();
  ~FullscreenArtist();

  // no copy/move (we store in the set by pointer below)
  FullscreenArtist(const FullscreenArtist&) = delete;
  FullscreenArtist(FullscreenArtist&&) = delete;
  FullscreenArtist& operator=(const FullscreenArtist&) = delete;
  FullscreenArtist& operator=(FullscreenArtist&&) = delete;


  // Fullscreen artists must override this function; when called it should disable the arist from drawing anything
  // fullscreen.
  virtual void disableFullscreenDrawing() = 0;
};

// Ensure no artist is drawing fullscreen currently.
void disableAllFullscreenArtists();

} // namespace polyscope

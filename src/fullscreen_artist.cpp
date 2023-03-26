// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/fullscreen_artist.h"

namespace polyscope {

// The static list
std::set<FullscreenArtist*> currentFullscreenArtists;

FullscreenArtist::FullscreenArtist() { currentFullscreenArtists.insert(this); }

FullscreenArtist::~FullscreenArtist() { currentFullscreenArtists.erase(this); }

void disableAllFullscreenArtists() {
  for (FullscreenArtist* a : currentFullscreenArtists) {
    a->disableFullscreenDrawing();
  }
}

} // namespace polyscope

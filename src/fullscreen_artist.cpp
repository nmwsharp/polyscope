// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include <algorithm>

#include "polyscope/fullscreen_artist.h"

namespace polyscope {

// The static list
std::vector<WeakHandle<FullscreenArtist>> existingFullscreenArtists;

FullscreenArtist::FullscreenArtist() {
  existingFullscreenArtists.emplace_back(this->getWeakHandle<FullscreenArtist>(this));
}

FullscreenArtist::~FullscreenArtist() {
  // the weak handle will become invalid, let it get lazily delete at some later time
}

void disableAllFullscreenArtists() {

  // "erase-remove idiom"
  // (remove list entries for which the view weak_ptr has .expired() == true)
  existingFullscreenArtists.erase(
      std::remove_if(existingFullscreenArtists.begin(), existingFullscreenArtists.end(),
                     [](const WeakHandle<FullscreenArtist>& entry) -> bool { return !entry.isValid(); }),
      existingFullscreenArtists.end());

  for (WeakHandle<FullscreenArtist>& a : existingFullscreenArtists) {
    a.get().disableFullscreenDrawing();
  }
}

} // namespace polyscope

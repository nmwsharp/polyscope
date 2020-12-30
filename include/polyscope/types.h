// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once
#include <string>

// Various types / enums / forward declarations which are broadly useful

namespace polyscope {
enum class BackgroundView { None = 0 };
enum class TransparencyMode { None = 0, Simple, Pretty };
enum class GroundPlaneMode {None, Tile, TileReflection, ShadowOnly };
enum class BackfacePolicy {Identical, Different, Cull};
}; // namespace polyscope

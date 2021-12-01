// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <tuple>

#include "imgui.h"


namespace polyscope {

// Default implementations of callbacks to set ImGui style / fonts
void configureImGuiStyle();
std::tuple<ImFontAtlas*, ImFont*, ImFont*> prepareImGuiFonts();

} // namespace polyscope

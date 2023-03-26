// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <tuple>

#include "imgui.h"


namespace polyscope {

// Default implementations of callbacks to set ImGui style / fonts
void configureImGuiStyle();
std::tuple<ImFontAtlas*, ImFont*, ImFont*> prepareImGuiFonts();

} // namespace polyscope

#pragma once

#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"

// Macro to construct shader strings. Unforunately, it eats line numbers.
// TODO one day, change the way the version is embedded, so we can put common shader source before the shader-specific
// source, rather than after. This will allow us to avoid forward-declaration of shared funciton.
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

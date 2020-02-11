#pragma once

#include "polyscope/render/engine.h"
#include "polyscope/render/shaders.h"

// Macro to construct shader strings. Unforunately, it eats line numbers.
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

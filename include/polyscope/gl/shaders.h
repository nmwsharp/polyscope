// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <cstdlib>
#include <string>
#include <vector>

// Make syntax  nicer like this, but we lose line numbers in GL debug output
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

namespace polyscope {
namespace gl {

} // namespace gl
} // namespace polyscope

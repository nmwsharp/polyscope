// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// This file defines common routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED
#include "polyscope/render/opengl/gl_engine.h"
#endif

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

extern const char* shaderCommonSource;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

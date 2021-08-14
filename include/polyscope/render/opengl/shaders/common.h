// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// This file defines common routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

namespace polyscope {
namespace render {
namespace backend_openGL3_glfw {

extern const char* shaderCommonSource;

} // namespace backend_openGL3_glfw
} // namespace render
} // namespace polyscope

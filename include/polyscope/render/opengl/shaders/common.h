// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

// This file defines common routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

namespace polyscope {
namespace render {
namespace backend_openGL3 {

extern const char* shaderCommonSource;

} // namespace backend_openGL3
} // namespace render
} // namespace polyscope

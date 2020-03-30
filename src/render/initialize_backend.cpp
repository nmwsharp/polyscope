#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

// Storage for the global engine pointer
Engine* engine = nullptr;

// Forward-declaration of initialize routines
// we don't want to just include the appropriate headers, because they may define conflicting symbols
namespace backend_openGL3_glfw {
void initializeRenderEngine();
}
namespace backend_openGL_mock {
void initializeRenderEngine();
}
// void initializeRenderEngine_openGL_mock();

void initializeRenderEngine(std::string backend) {

  // Handle default backends
  // (the string is getting overwritten, so lower on the list means higher priority)
  if (backend == "") {

#ifdef POLYSCOPE_BACKEND_OPENGL_MOCK_ENABLED
    // Don't set it one by default, since it's probably a mistake; better to throw the exception below.
    // backend = "mock_openGL";
#endif

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED
    backend = "openGL3_glfw";
#endif

    if (backend == "") {
      throw std::runtime_error("no Polyscope backends available");
    }
  }

  // Initialize the appropriate backend
  if (backend == "openGL3_glfw") {
    backend_openGL3_glfw::initializeRenderEngine();
  } else if (backend == "openGL_mock") {
    backend_openGL_mock::initializeRenderEngine();
  } else {
    throw std::runtime_error("unrecognized Polyscope backend " + backend);
  }
}

} // namespace render
} // namespace polyscope


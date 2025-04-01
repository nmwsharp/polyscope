// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/messages.h"
#include "polyscope/options.h"
#include "polyscope/render/engine.h"

#include <string>

namespace polyscope {
namespace render {

// Storage for the global engine pointer
Engine* engine = nullptr;

// Backend we initialized with; written once below
std::string engineBackendName = "";

// Forward-declaration of initialize routines
// we don't want to just include the appropriate headers, because they may define conflicting symbols
namespace backend_openGL3 {
void initializeRenderEngine_glfw();
void initializeRenderEngine_egl();
} // namespace backend_openGL3
namespace backend_openGL_mock {
void initializeRenderEngine();
}

void initializeRenderEngine(std::string backend) {

  // Handle default backends
  if (backend == "") {
    backend = "auto"; // treat "" as "auto"
  }

  engineBackendName = backend;

  // Initialize the appropriate backend
  if (backend == "openGL3_glfw") {
    backend_openGL3::initializeRenderEngine_glfw();
  } else if (backend == "openGL3_egl") {
    backend_openGL3::initializeRenderEngine_egl();
  } else if (backend == "openGL_mock") {
    backend_openGL_mock::initializeRenderEngine();
  } else if (backend == "auto") {

    // Attempt to automatically initialize by trynig

    bool initSucces = false;
    std::string extraMessage = "";

#ifdef POLYSCOPE_BACKEND_OPENGL3_GLFW_ENABLED
    // First try GLFW, if available
    engineBackendName = "openGL3_glfw";
    try {
      backend_openGL3::initializeRenderEngine_glfw();
      initSucces = true;
    } catch (const std::exception& e) {
      if (options::verbosity > 0) {
        info("Automatic initialization status: could not initialize backend [openGL3_glfw].");
      }
    }
    if (initSucces) return;
#endif

#ifdef POLYSCOPE_BACKEND_OPENGL3_EGL_ENABLED

    if (options::allowHeadlessBackends) {

      // Then, try EGL if available
      engineBackendName = "openGL3_egl";
      try {
        backend_openGL3::initializeRenderEngine_egl();
        initSucces = true;
      } catch (const std::exception& e) {
        if (options::verbosity > 0) {
          info("Automatic initialization status: could not initialize backend [openGL3_egl].");
        }
      }
      if (initSucces) {
        if (options::verbosity > 0) {
          info("Automatic initialization yielded a headless backend, likely because no display was found. Rendering is "
               "supported, but no interactive windows can be "
               "created. See polyscope.run/features/headless_rendering/");
        }
        return;
      }

    } else {
      extraMessage = "Polyscope was compiled with support for the headless EGL backend, but "
                     "allowHeadlessBackends=false. Set it to true to attempt "
                     "headless initialization.";
    }

#endif

    // Don't bother trying the 'mock' backend, it is unlikely to be what the user wants from the 'auto' option

    // Failure
    exception("Automatic initialization: no Polyscope backends could be initialized successfully." + extraMessage);

  } else {
    exception("unrecognized Polyscope backend " + backend);
  }
}

} // namespace render
} // namespace polyscope

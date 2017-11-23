#pragma once

#include "polyscope/options.h"
#include "polyscope/gl/gl_utils.h"

namespace polyscope {

    // Initialize polyscope, including windowing system and openGL. Should be
    // called exactly once at the beginning of a program. If initialization
    // fails in any way, an exception will be thrown.
    void init();

    // Give control to the polyscope GUI. Blocks until the user returns control via the GUI,
    // possibly by exiting the window.
    void show();


    // === Global variables ===
    namespace state {

        // has polyscope::init() been called?
        extern bool initialized; 

    } // namespace state

} // namespace polyscope
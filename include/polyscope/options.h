// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <string>

namespace polyscope {
namespace options {

// A general name to use when referring to the program in window headings.
extern std::string programName;

// How much should polyscope print to std::out?
// 0 --> none
// 1 --> some
// > 1 --> a lot
extern int verbosity;

// A string to prefex all messages printed to stdout
extern std::string printPrefix;

// Should errors throw exceptions, or just display? (default false)
extern bool errorsThrowExceptions;

// Don't let the main loop run at more than this speed. (-1 disables) (default: 60)
extern int maxFPS;

// Read preferences (window size, etc) from startup file, write to same file on exit (default: true)
extern bool usePrefsFile;

// Should we redraw every frame, even if not requested? (default: false)
extern bool alwaysRedraw;

// Should we center/scale every structure after it is loaded up (default: false)
extern bool autocenterStructures;
extern bool autoscaleStructures;

// Should the user call back start out with an imgui window context open (default: true)
extern bool openImGuiWindowForUserCallback;

// If true, the user callback will be invoked for nested calls to polyscope::show(), otherwise not (default: false)
extern bool invokeUserCallbackForNestedShow;

// Enables optional error checks in the rendering system
extern bool enableRenderErrorChecks;

} // namespace options
} // namespace polyscope

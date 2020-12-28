// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/options.h"

namespace polyscope {
namespace options {

std::string programName = "Polyscope";
int verbosity = 1;
std::string printPrefix = "[polyscope] ";
bool errorsThrowExceptions = false;
bool debugDrawPickBuffer = false;
int maxFPS = 60;
bool usePrefsFile = true;
bool initializeWithDefaultStructures = true;
bool alwaysRedraw = false;
bool autocenterStructures = false;
bool autoscaleStructures = false;
bool openImGuiWindowForUserCallback = true;
bool invokeUserCallbackForNestedShow = false;
TransparencyMode transparencyMode = TransparencyMode::None;
int transparencyRenderPasses = 8;

// enabled by default in debug mode
#ifndef NDEBUG
bool enableRenderErrorChecks = false;
#else
bool enableRenderErrorChecks = true;
#endif

} // namespace options
} // namespace polyscope

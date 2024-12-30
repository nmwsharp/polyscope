// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/options.h"
#include "polyscope/imgui_config.h"

namespace polyscope {
namespace options {

std::string programName = "Polyscope";
int verbosity = 2;
std::string printPrefix = "[polyscope] ";
bool allowHeadlessBackends = false;
bool errorsThrowExceptions = false;
bool debugDrawPickBuffer = false;
int maxFPS = 60;
bool enableVSync = true;
bool usePrefsFile = false;
bool initializeWithDefaultStructures = true;
bool alwaysRedraw = false;
bool autocenterStructures = false;
bool autoscaleStructures = false;
bool automaticallyComputeSceneExtents = true;
bool invokeUserCallbackForNestedShow = false;
bool giveFocusOnShow = false;
bool hideWindowAfterShow = true;
bool warnForInvalidValues = true;
bool displayMessagePopups = true;

bool screenshotTransparency = true;
std::string screenshotExtension = ".png";

// == Scene options

// Ground plane / shadows
bool groundPlaneEnabled = true;
GroundPlaneMode groundPlaneMode = GroundPlaneMode::TileReflection;
GroundPlaneHeightMode groundPlaneHeightMode = GroundPlaneHeightMode::Automatic;
ScaledValue<float> groundPlaneHeightFactor = 0;
float groundPlaneHeight = 0.;
int shadowBlurIters = 2;
float shadowDarkness = 0.25;

// Rendering options

int ssaaFactor = 1;

// Transparency
TransparencyMode transparencyMode = TransparencyMode::None;
int transparencyRenderPasses = 8;

// === Advanced ImGui configuration

bool buildGui = true;
bool userGuiIsOnRightSide = true;
bool buildDefaultGuiPanels = true;
bool renderScene = true;
bool openImGuiWindowForUserCallback = true;
std::function<void()> configureImGuiStyleCallback = configureImGuiStyle;
std::function<std::tuple<ImFontAtlas*, ImFont*, ImFont*>()> prepareImGuiFontsCallback = prepareImGuiFonts;

// Backend and low-level options
int eglDeviceIndex = -1; // means "try all of them"

// enabled by default in debug mode
#ifndef NDEBUG
bool enableRenderErrorChecks = false;
#else
bool enableRenderErrorChecks = true;
#endif

} // namespace options
} // namespace polyscope

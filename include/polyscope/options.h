// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <functional>
#include <string>
#include <tuple>

#include "imgui.h"

#include "polyscope/scaled_value.h"
#include "polyscope/types.h"


namespace polyscope {
namespace options { // A general name to use when referring to the program in window headings.
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

// If true, Polyscope will automatically compute state::boundingBox and state::lengthScale parameters according to the
// registered structures, and update them whenever structures are added chagned. If false, the bounding box and length
// scale are left unchanged. If set to false before the first structure is registered, the user is required to set the
// bounding box and length scale manually. (default: true)
extern bool automaticallyComputeSceneExtents;

// If true, the user callback will be invoked for nested calls to polyscope::show(), otherwise not (default: false)
extern bool invokeUserCallbackForNestedShow;

// If true, focus the Polyscope window when shown (default: false)
extern bool giveFocusOnShow;

// === Scene options

// Behavior of the ground plane
extern GroundPlaneMode groundPlaneMode;
extern bool groundPlaneEnabled; // deprecated, but kept and respected for compatability. use groundPlaneMode.
extern ScaledValue<float> groundPlaneHeightFactor;
extern int shadowBlurIters;
extern float shadowDarkness;

extern bool screenshotTransparency;     // controls whether screenshots taken by clicking the GUI button have a
                                        // transparent background
extern std::string screenshotExtension; // sets the extension used for automatically-numbered screenshots (e.g. by
                                        // clicking the GUI button)

// === Rendering parameters

// SSAA scaling in pixel multiples
extern int ssaaFactor;

// Transparency settings for the renderer
extern TransparencyMode transparencyMode;
extern int transparencyRenderPasses;

// === Advanced ImGui configuration

// If false, Polyscope will not create any ImGui UIs at all, but will still set up ImGui and invoke its render steps
// each frame. The allows advanced users to create their own UIs totally from scratch and circumvent the standard
// Polyscope UIs. (default: true)
extern bool buildGui;

// Should the user call back start out with an imgui window context open (default: true)
extern bool openImGuiWindowForUserCallback;

// A callback function which will be invoked when an ImGui context is created (which may happen several times as
// Polyscope runs). By default, this is set to invoke `configureImGuiStyle()` from Polyscope's imgui_config.cpp, but you
// may assign your own function to create custom styles. If this callback is null, the default ImGui style will be used.
extern std::function<void()> configureImGuiStyleCallback;

// A callback function which will be invoked exactly once during initialization to construct a font atlas for ImGui to
// use. The callback should return a tuple of three pointers: a newly created global shared font atlas, a regular font,
// and a mono font. By default, this is set to invoke prepareImGuiFonts() from Polyscope's imgui_config.cpp, but you may
// assign your own function to create custom styles. If this callback is null, default fonts will be used.
extern std::function<std::tuple<ImFontAtlas*, ImFont*, ImFont*>()> prepareImGuiFontsCallback;


// === Debug options

// Enables optional error checks in the rendering system
extern bool enableRenderErrorChecks;

// Render the pick buffer to screen rather than the regular scene
extern bool debugDrawPickBuffer;

} // namespace options
} // namespace polyscope

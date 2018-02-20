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
    
    // Show empty structure types from startup (default: true)
    extern bool initializeWithDefaultStructures;
    
    // Automatically center structures upon registration (default: false)
    extern bool autocenterStructures;

} // namespace options
} // namespace polyscope

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
    extern std::string printPrefx;

    // Should errors be exceptions or printed messages?
    extern bool execptionOnError;
    

} // namespace options
} // namespace polyscope
#pragma once

#include <string>

namespace polyscope {
namespace utilities {


// === String related utilities

// Attempt to get a user-friendly name for a file from its base name
std::string guessNiceNameFromPath(std::string fullname);

// Print large integers in a user-friendly way (like "37.5B")
std::string prettyPrintCount(size_t count);

} // namespace utilities
} // namespace polyscope
// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <string>

namespace polyscope {

// == Register various kinds of messages

// General details, things you would print to stdout. For now, that's exactly what it does
void info(std::string message);

// Non-fatal warnings. Warnings with the same base message are batched together, so the UI doesn't get completely
// overwhelmed if you call this in a dense loop.
void warning(std::string baseMessage, std::string detailMessage = "");

// Errors which are certainly big problems, and we may or may not be able to recover from. Blocks the UI
void error(std::string message);

// Errors which are so bad we won't even try to recover from them. Displays to the user before exiting the program.
// Internally used for uncaught exceptions.
void terminatingError(std::string message);

// Wrapper used by polyscope to throw exceptions
void exception(std::string message);

// Process any warnings that have accumulated, showing them to the user and clearing the queue.
void showDelayedWarnings();
void clearMessages();
} // namespace polyscope

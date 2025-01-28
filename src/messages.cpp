// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/messages.h"

#include "imgui.h"
#include "polyscope/polyscope.h"

#include <deque>

namespace polyscope {

// Helpers
namespace {


// A queue of warnings
struct WarningMessage {
  std::string baseMessage;
  std::string detailMessage;
  int repeatCount;
};
bool showingWarning = false;

// A queue of warning messages to show
std::deque<WarningMessage> warningMessages;

void buildErrorUI(std::string message, bool fatal) {

  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5, 0.5));
  std::string errorPopupString = fatal ? "FATAL ERROR" : "ERROR";
  ImGui::OpenPopup(errorPopupString.c_str());

  // Nice size for error modal
  ImVec2 errorTextSize = ImGui::CalcTextSize(message.c_str());
  ImVec2 errorModalSize(std::max(view::windowWidth / 5.0f, std::min(errorTextSize.x + 50, view::windowWidth / 2.0f)),
                        0);
  ImVec2 errorModalPos((view::windowWidth - errorModalSize.x) / 2, view::windowHeight / 3);

  ImGui::SetNextWindowSize(errorModalSize);
  ImGui::SetNextWindowPos(errorModalPos, ImGuiCond_Always);
  ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(170. / 255., 0, 0, 1.0));
  if (ImGui::BeginPopupModal(errorPopupString.c_str(), NULL, ImGuiWindowFlags_NoMove)) {

    // Nice text sizing
    float offset = (errorModalSize.x - errorTextSize.x) / 2.0 - 5;
    offset = std::max(offset, 0.0f);
    bool doIndent = offset > 0;
    if (doIndent) {
      ImGui::Indent(offset);
    }

    // Make text
    ImGui::TextWrapped("%s", message.c_str());
    if (doIndent) {
      ImGui::Unindent(offset);
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // Nice button sizing
    float buttonWidth = 120;
    float buttonOffset = (errorModalSize.x - buttonWidth) / 2.0;
    buttonOffset = std::max(buttonOffset, 0.0f);
    doIndent = buttonOffset > 0;
    if (doIndent) {
      ImGui::Indent(buttonOffset);
    }

    // Make a button
    if (ImGui::Button("My bad.", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
      popContext();
      ImGui::CloseCurrentPopup();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("(space to dismiss)");

    if (doIndent) {
      ImGui::Unindent(buttonOffset);
    }
  }

  ImGui::EndPopup();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

void buildWarningUI(std::string warningBaseString, std::string warningDetailString, int nRepeats) {

  // Center modal window titles
  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5, 0.5));

  // == Build the warning dialog
  ImGui::OpenPopup("WARNING");


  std::string warningRepeatString = "";
  if (nRepeats > 0) {
    warningRepeatString = "(and " + std::to_string(nRepeats) + " similar warnings)";
  }

  // Nice size for warning modal
  ImVec2 warningBaseTextSize = ImGui::CalcTextSize(warningBaseString.c_str());
  ImVec2 warningDetailTextSize = ImGui::CalcTextSize(warningDetailString.c_str());
  ImVec2 warningRepeatTextSize = ImGui::CalcTextSize(warningRepeatString.c_str());
  ImVec2 warningMaxTextSize(
      std::max(warningBaseTextSize.x, std::max(warningDetailTextSize.x, warningRepeatTextSize.x)),
      std::max(warningBaseTextSize.y, std::max(warningDetailTextSize.y, warningRepeatTextSize.y)));
  ImVec2 warningModalSize(
      std::max(view::windowWidth / 5.0f, std::min(warningMaxTextSize.x + 50, view::windowWidth / 2.0f)), 0);
  ImVec2 warningModalPos((view::windowWidth - warningModalSize.x) / 2, view::windowHeight / 3);

  ImGui::SetNextWindowSize(warningModalSize);
  ImGui::SetNextWindowPos(warningModalPos, ImGuiCond_Always);
  ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(190. / 255., 166. / 255., 0, 1.0));
  if (ImGui::BeginPopupModal("WARNING", NULL, ImGuiWindowFlags_NoMove)) {

    // Nice text sizing
    float offset = (warningModalSize.x - warningBaseTextSize.x) / 2.0 - 5;
    offset = std::max(offset, 0.0f);
    bool doIndent = offset > 0;
    if (doIndent) {
      ImGui::Indent(offset);
    }

    // Make base text
    ImGui::TextWrapped("%s", warningBaseString.c_str());
    if (doIndent) {
      ImGui::Unindent(offset);
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // Nice text sizing
    if (warningDetailString != "") {
      offset = (warningModalSize.x - warningDetailTextSize.x) / 2.0 - 5;
      offset = std::max(offset, 0.0f);
      doIndent = offset > 0;
      if (doIndent) {
        ImGui::Indent(offset);
      }

      // Make detail text
      ImGui::TextWrapped("%s", warningDetailString.c_str());
      if (doIndent) {
        ImGui::Unindent(offset);
      }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // Nice text sizing
    if (warningRepeatString != "") {
      offset = (warningModalSize.x - warningRepeatTextSize.x) / 2.0 - 5;
      offset = std::max(offset, 0.0f);
      doIndent = offset > 0;
      if (doIndent) {
        ImGui::Indent(offset);
      }

      // Make detail text
      ImGui::TextWrapped("%s", warningRepeatString.c_str());
      if (doIndent) {
        ImGui::Unindent(offset);
      }

      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Spacing();
    }

    // Nice button sizing
    float buttonWidth = 120;
    float buttonOffset = (warningModalSize.x - buttonWidth) / 2.0;
    buttonOffset = std::max(buttonOffset, 0.0f);
    doIndent = buttonOffset > 0;
    if (doIndent) {
      ImGui::Indent(buttonOffset);
    }

    // Make a button
    if (ImGui::Button("This is fine.", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
      ImGui::CloseCurrentPopup();
      popContext();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("(space to dismiss)");

    if (doIndent) {
      ImGui::Unindent(buttonOffset);
    }
  }

  ImGui::EndPopup();
  ImGui::PopStyleColor();

  // Window title
  ImGui::PopStyleVar();
}
} // namespace


void info(std::string message) { info(0, message); }
void info(int verbosityLevel, std::string message) {
  if (options::verbosity > verbosityLevel) {
    std::cout << options::printPrefix << message << std::endl;
  }
}

void error(std::string message) {
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "[ERROR] " << message << std::endl;
  }

  // Enter a modal UI loop showing the error
  if (options::displayMessagePopups && isInitialized() && !isHeadless()) {
    auto func = std::bind(buildErrorUI, message, false);
    pushContext(func, false);
  }

  if (options::errorsThrowExceptions) {
    throw std::logic_error(options::printPrefix + message);
  }
}

void exception(std::string message) {

  message = options::printPrefix + " [EXCEPTION] " + message;

  if (options::verbosity > 0) {
    std::cout << message << std::endl;
  }

  throw std::runtime_error(message);
}

void terminatingError(std::string message) {
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "[ERROR] " << message << std::endl;
  }

  // Enter a modal UI loop showing the warning
  if (options::displayMessagePopups && isInitialized() && !isHeadless()) {
    auto func = std::bind(buildErrorUI, message, true);
    pushContext(func, false);
  }

  // Quit the program
  shutdown(true);
  std::exit(-1);
}

void warning(std::string baseMessage, std::string detailMessage) {

  // print to stdout
  if (options::verbosity > 0) {
    std::cout << options::printPrefix << "[WARNING] " << baseMessage;
    if (detailMessage != "") std::cout << " --- " << detailMessage;
    std::cout << std ::endl;
  }

  // Look for a message with the same name
  bool found = false;
  for (WarningMessage& w : warningMessages) {
    if (w.baseMessage == baseMessage) {
      found = true;
      w.repeatCount++;
    }
  }

  // Create a new message
  if (!found) {
    warningMessages.push_back(WarningMessage{baseMessage, detailMessage, 0});
  }
}

void showDelayedWarnings() {

  if (showingWarning) {
    return;
  }

  while (warningMessages.size() > 0) {
    showingWarning = true;
    WarningMessage& currMessage = warningMessages.front();

    // Enter a modal UI loop showing the warning
    if (options::displayMessagePopups && isInitialized() && !isHeadless()) {
      auto func =
          std::bind(buildWarningUI, currMessage.baseMessage, currMessage.detailMessage, currMessage.repeatCount);
      pushContext(func, false);
    }

    warningMessages.pop_front();
    showingWarning = false;
  }
}

void clearMessages() { warningMessages.clear(); }

} // namespace polyscope

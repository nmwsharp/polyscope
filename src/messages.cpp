#include "polyscope/messages.h"

#include "imgui.h"
#include "polyscope/polyscope.h"

#include <deque>

using std::cout;
using std::endl;
using std::string;

namespace polyscope {

// Helper to hold warnings
namespace {
struct WarningMessage {
  std::string baseMessage;
  std::string detailMessage;
  int repeatCount;
};

// A queue of warning messages to show
std::deque<WarningMessage> warningMessages;

bool currentlyShowingError = false;
bool currentErrorIsFatal = false;
std::string errorString = "";

} // namespace

bool messageIsBlockingScreen() { return currentlyShowingError || warningMessages.size() > 0; }

void info(std::string message) { cout << options::printPrefix << message << endl; }

void error(std::string message) {

  currentlyShowingError = true;
  currentErrorIsFatal = false;
  errorString = message;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();

  // Re-enter main loop
  while (currentlyShowingError) {
    mainLoopIteration();
  }

  // Restore the old context
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);

  std::cout << options::printPrefix << "[ERROR] " << message << std::endl;
  if (options::errorsThrowExceptions) {
    throw std::logic_error(options::printPrefix + message);
  }
}

void terminatingError(std::string message) {

  currentlyShowingError = true;
  currentErrorIsFatal = true;
  errorString = message;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();

  // Re-enter main loop
  while (currentlyShowingError) {
    mainLoopIteration();
  }

  // Restore the old context
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);

  std::cout << options::printPrefix << "[ERROR] " << message << std::endl;

  // Quit the program
  shutdown(-1);
}

void warning(std::string baseMessage, std::string detailMessage) {

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


void buildMessagesUI() {

  // Center modal window titles
  ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5, 0.5));

  std::string errorPopupString = currentErrorIsFatal ? "FATAL ERROR" : "ERROR";

  // == Build the error dialog
  if (currentlyShowingError) {

    ImGui::OpenPopup(errorPopupString.c_str());

    // Nice size for error modal
    ImVec2 errorTextSize = ImGui::CalcTextSize(errorString.c_str());
    ImVec2 errorModalSize(std::max(view::windowWidth / 5.0f, std::min(errorTextSize.x + 50, view::windowWidth / 2.0f)),
                          0);

    ImGui::SetNextWindowSize(errorModalSize);
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
      ImGui::TextWrapped("%s", errorString.c_str());
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
      if (ImGui::Button("My bad.", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed((int)' ')) {
        currentlyShowingError = false;
        ImGui::CloseCurrentPopup();
      }
      if (doIndent) {
        ImGui::Unindent(buttonOffset);
      }
    }

    ImGui::EndPopup();
    ImGui::PopStyleColor();
  }

  // == Build the warning dialog
  if (!currentlyShowingError && warningMessages.size() > 0) {
    ImGui::OpenPopup("WARNING");

    // Get the current warning
    WarningMessage& currMessage = warningMessages.front();

    string warningBaseString = currMessage.baseMessage;
    string warningDetailString = currMessage.detailMessage;
    string warningRepeatString = "";
    if (currMessage.repeatCount > 0) {
      warningRepeatString = "(and " + std::to_string(currMessage.repeatCount) + " similar warnings)";
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

    ImGui::SetNextWindowSize(warningModalSize);
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
      if (ImGui::Button("This is fine.", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed((int)' ')) {
        warningMessages.pop_front();
        ImGui::CloseCurrentPopup();
      }
      if (doIndent) {
        ImGui::Unindent(buttonOffset);
      }
    }

    ImGui::EndPopup();
    ImGui::PopStyleColor();
  }

  // Window title
  ImGui::PopStyleVar();
}


} // namespace polyscope

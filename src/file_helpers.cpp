#include "polyscope/file_helpers.h"

#include "imgui.h"
#include "polyscope/polyscope.h"

namespace polyscope {

namespace {

void filenamePromptCallback(char* buff, size_t len) {

  static bool windowOpen = true;
  ImGui::Begin("Enter filename", &windowOpen, ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::PushItemWidth(500);
  ImGui::InputText("##filename", buff, len);

  if (ImGui::Button("Ok")) {
    focusedPopupUI = nullptr;
  }

  ImGui::SameLine();

  if (ImGui::Button("Cancel")) {
    sprintf(buff, "");
    focusedPopupUI = nullptr;
  }

  ImGui::PopItemWidth();
  ImGui::End();
}
} // namespace


std::string promptForFilename() {

  using namespace std::placeholders;

  // Create a new context
  ImGuiContext* oldContext = ImGui::GetCurrentContext();
  ImGuiContext* newContext = ImGui::CreateContext(getGlobalFontAtlas());
  ImGui::SetCurrentContext(newContext);
  initializeImGUIContext();

  // Register the callback which creates the UI and does the hard work
  // char textBuff[1024];
  char* textBuff = new char[1024];
  sprintf(textBuff, "out");
  auto func = std::bind(filenamePromptCallback, textBuff, 1024);
  focusedPopupUI = func;

  // Re-enter main loop
  while (focusedPopupUI) {
    mainLoopIteration();
  }

  std::string stringOut(textBuff);

  // Restore the old context
  ImGui::SetCurrentContext(oldContext);
  ImGui::DestroyContext(newContext);

  return stringOut;
}
} // namespace polyscope

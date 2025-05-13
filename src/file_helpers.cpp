// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/file_helpers.h"

#include "imgui.h"
#include "polyscope/polyscope.h"

namespace polyscope {

namespace {

void filenamePromptCallback(char* buff, size_t len) {

  static bool windowOpen = true;
  ImGui::Begin("Enter filename", &windowOpen, ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::PushItemWidth(500 * options::uiScale);
  ImGui::InputText("##filename", buff, len);

  if (ImGui::Button("Ok")) {
    popContext();
  }

  ImGui::SameLine();

  if (ImGui::Button("Cancel")) {
    // sprintf(buff, "");
    buff[0] = 0; // because apparently gcc this this is clearer than that ^^^?
    popContext();
  }

  ImGui::PopItemWidth();
  ImGui::End();
}
} // namespace


std::string promptForFilename(std::string initName) {

  using namespace std::placeholders;

  // Register the callback which creates the UI and does the hard work
  const size_t BUFF_LEN = 2048;
  char* textBuff = new char[BUFF_LEN];
  snprintf(textBuff, BUFF_LEN, "%s", initName.c_str());
  auto func = std::bind(filenamePromptCallback, textBuff, BUFF_LEN);
  pushContext(func);

  std::string stringOut(textBuff);
  delete[] textBuff;

  return stringOut;
}
} // namespace polyscope

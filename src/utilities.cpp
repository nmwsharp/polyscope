// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/utilities.h"


#include <cmath>
#include <vector>

#include "imgui.h"

#include "polyscope/messages.h"


namespace polyscope {

// Globals for random utilities
std::random_device util_random_device;
std::mt19937 util_mersenne_twister(util_random_device());

std::string guessNiceNameFromPath(std::string fullname) {
  size_t startInd = 0;
  for (std::string sep : {"/", "\\"}) {
    size_t pos = fullname.rfind(sep);
    if (pos != std::string::npos) {
      startInd = std::max(startInd, pos + 1);
    }
  }

  size_t endInd = fullname.size();
  for (std::string sep : {"."}) {
    size_t pos = fullname.rfind(sep);
    if (pos != std::string::npos) {
      endInd = std::min(endInd, pos);
    }
  }

  if (startInd >= endInd) {
    return fullname;
  }

  std::string niceName = fullname.substr(startInd, endInd - startInd);
  return niceName;
}

void validateName(const std::string& name) {
  if (name == "") exception("name must not be the empty string");
  if (name.find("#") != std::string::npos) exception("name must not contain '#' characters");
}

std::tuple<std::string, std::string> splitExt(std::string f) {
  auto p = f.find_last_of(".");
  return std::tuple<std::string, std::string>{f.substr(0, p), f.substr(p, std::string::npos)};
}

void splitTransform(const glm::mat4& trans, glm::mat3x4& R, glm::vec3& T) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      R[i][j] = trans[i][j];
    }
    T[i] = trans[3][i];
  }
}

glm::mat4 buildTransform(const glm::mat3x4& R, const glm::vec3& T) {
  glm::mat4 trans(1.0);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      trans[i][j] = R[i][j];
    }
    trans[3][i] = T[i];
  }

  return trans;
}


std::string prettyPrintCount(size_t count) {

  int nDigits = 1;
  if (count > 0) {
    nDigits = 1 + std::floor(std::log10(count));
  }

  // Print small values exactly
  if (nDigits <= 4) {
    return std::to_string(count);
  }

  std::vector<std::string> postFixes = {"", "K", "M", "B", "T"};
  size_t iPostfix = 0;
  size_t iPow = 0;
  double countD = count;

  while (nDigits > 3) {
    count /= 1000;
    countD /= 1000;
    iPostfix++;
    iPow += 3;
    nDigits -= 3;
  }

  // Get a postfix, either as a predefined character or scientific notation
  std::string postfix;
  if (iPostfix < postFixes.size()) {
    postfix = postFixes[iPostfix];
  } else {
    postfix = "*10^" + std::to_string(iPow);
  }

  // Build the actual string
  char buf[50];
  if (nDigits == 1) {
    snprintf(buf, 50, "%2.2f%s", countD, postfix.c_str());
    return std::string(buf);
  } else if (nDigits == 2) {
    snprintf(buf, 50, "%2.1f%s", countD, postfix.c_str());
    return std::string(buf);
  } else /*(nDigits == 3) */ {
    snprintf(buf, 50, "%2.0f%s", countD, postfix.c_str());
    return std::string(buf);
  }
}

void ImGuiHelperMarker(const char* text) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

} // namespace polyscope

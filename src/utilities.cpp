// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/utilities.h"

#include <cmath>
#include <vector>


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

std::string prettyPrintCount(size_t count) {

  /*
  // Pretty print test
  size_t num = 0;
  size_t mult = 1;
  for(int i = 0; i < 18; i++) {
    cout << num <<  " --> |" << polyscope::utilities::prettyPrintCount(num) << "|" <<  endl;
    mult *= 10;
    num += mult * randomInt(0,9);
  }
  */

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

} // namespace polyscope

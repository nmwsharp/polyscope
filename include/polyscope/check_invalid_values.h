// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <polyscope/numeric_helpers.h>
#include <polyscope/options.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <type_traits>

namespace polyscope {

template <typename T>
void checkInvalidValues(std::string name, const std::vector<T>& data) {
  if (options::warnForInvalidValues) {
    for (const T& val : data) {
      if (!allComponentsFinite(val)) {
        warning("Invalid +-inf or NaN values detected",
                "in buffer: " + name + "\n(set warnForInvalidValues=false to disable)");
        break;
      }
    }
  }
}

} // namespace polyscope
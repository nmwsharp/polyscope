#pragma once

#include <complex>
#include <type_traits>
#include <vector>

namespace polyscope {

// Check that a data array has the expected size
template <class T>
void validateSize(const T& inputData, std::vector<size_t> expectedSizes, std::string errorName = "") {

  // No-op if no sizes given
  if (expectedSizes.size() == 0) {
    return;
  }

  // Simpler error if only one size
  if (expectedSizes.size() == 1) {
    if (inputData.size() != expectedSizes[0]) {
      error("Size validation failed on data array " + errorName + ". Expected size " +
            std::to_string(expectedSizes[0]) + " but has size " + std::to_string(inputData.size()));
    }
  }
  // General case
  else {

    // Return success if any sizes match
    for (size_t possibleSize : expectedSizes) {
      if (inputData.size() == possibleSize) {
        return;
      }
    }

    // Build a useful error message
    std::string sizesStr = "{";
    for (size_t possibleSize : expectedSizes) {
      sizesStr += std::to_string(possibleSize) + ",";
    }
    sizesStr += "}";

    error("Size validation failed on data array " + errorName + ". Expected size in " + sizesStr + " but has size " +
          std::to_string(inputData.size()));
  }
}

// Pass through to general version
template <class T>
void validateSize(const T& inputData, size_t expectedSize, std::string errorName = "") {
  validateSize<T>(inputData, std::vector<size_t>{expectedSize}, errorName);
}


// Convert an array of scalar types
template <class D, class T>
std::vector<D> standardizeArray(const T& inputData) {

  // Copy data
  std::vector<D> dataOut(inputData.size());
  for (size_t i = 0; i < inputData.size(); i++) {
    dataOut[i] = inputData[i];
  }

  return dataOut;
}

// Convert between various low dimensional vector types
template <class T, class D, int N>
inline D accessVectorLikeValue(T& inVal, size_t ind) {
  return inVal[ind];
}
template <>
inline double accessVectorLikeValue<std::complex<double>, double, 2>(std::complex<double>& inVal, size_t ind) {
  if (ind == 0) {
    return inVal.real();
  } else if (ind == 1) {
    return inVal.imag();
  }
  return -1.0;
}

// Convert an array of low-dimensional vector types
template <class D, class T, int N>
std::vector<D> standardizeVectorArray(const T& inputData) {

  // Copy data
  std::vector<D> dataOut(inputData.size());
  typedef typename std::remove_reference<decltype(dataOut[0][0])>::type OutScalarT;
  for (size_t i = 0; i < inputData.size(); i++) {
    for (size_t j = 0; j < N; j++) {
      dataOut[i][j] = accessVectorLikeValue<decltype(inputData[0]), OutScalarT, N>(inputData[i], j);
    }
  }

  return dataOut;
}
}

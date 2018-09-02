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
template <class T, class D>
inline D accessVectorLikeValue(T& inVal, size_t ind) {
  return inVal[ind];
}
template <class T, class D>
inline D accessVectorLikeValue(const T& inVal, size_t ind) {
  return inVal[ind];
}

// Template overload to access the two elements of std::complex
template <>
inline double accessVectorLikeValue<std::complex<double>, double>(std::complex<double>& inVal, size_t ind) {
  return reinterpret_cast<double(&)[2]>(inVal)[ind]; // guaranteed to work by the standard
}
template <>
inline float accessVectorLikeValue<const std::complex<double>, float>(const std::complex<double>& inVal, size_t ind) {
  return reinterpret_cast<const double(&)[2]>(inVal)[ind]; // guaranteed to work by the standard
}

// Convert an array of low-dimensional vector types. Outer type must support size() method. Inner type dimensions are
// not checked, and must match expected inner dimension N.
template <class D, class T, int N>
std::vector<D> standardizeVectorArray(const T& inputData) {

  // Copy data
  std::vector<D> dataOut(inputData.size());
  typedef typename std::remove_reference<decltype(dataOut[0][0])>::type OutScalarT;
  for (size_t i = 0; i < inputData.size(); i++) {
    for (size_t j = 0; j < N; j++) {
      dataOut[i][j] = accessVectorLikeValue<typename std::remove_reference<decltype(inputData[0])>::type, OutScalarT>(inputData[i], j);
    }
  }

  return dataOut;
}

// Convert a nested array where the inner types have variable length. Inner and outer types must support a size()
// method. Always returns a std::vector<std::vector<D>>
template <class D, class T>
std::vector<std::vector<D>> standardizeNestedList(const T& inputData) {

  // Copy data
  std::vector<std::vector<D>> dataOut(inputData.size());
  for (size_t i = 0; i < inputData.size(); i++) {
    size_t N = inputData[i].size();
    dataOut[i].resize(N);
    for (size_t j = 0; j < N; j++) {
      dataOut[i][j] = accessVectorLikeValue<decltype(inputData[0]), D>(inputData[i], j);
    }
  }

  return dataOut;
}

} // namespace polyscope

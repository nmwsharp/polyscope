#pragma once

#include <complex>
#include <type_traits>
#include <vector>

namespace polyscope {

// Convert an array of scalar types
template <class D, class T>
std::vector<D> standardizeArray(const T& inputData, size_t expectedSize, std::string errorName = "") {
  // Validate size
  if (inputData.size() != expectedSize) {
    throw std::runtime_error("Cannnot process array " + errorName + ". Expected size " + std::to_string(expectedSize) +
                             " but has size " + std::to_string(inputData.size()));
  }

  // Copy data
  std::vector<D> dataOut(expectedSize);
  for (size_t i = 0; i < expectedSize; i++) {
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
std::vector<D> standardizeVectorArray(const T& inputData, size_t expectedSize, std::string errorName = "") {
  // Validate size
  if (inputData.size() != expectedSize) {
    throw std::runtime_error("Cannnot process array " + errorName + ". Expected size " + std::to_string(expectedSize) +
                             " but has size " + std::to_string(inputData.size()));
  }

  // Copy data
  std::vector<D> dataOut(expectedSize);
  typedef typename std::remove_reference<decltype(dataOut[0][0])>::type OutScalarT;
  for (size_t i = 0; i < expectedSize; i++) {
    for (size_t j = 0; j < N; j++) {
      dataOut[i][j] = accessVectorLikeValue<decltype(inputData[0]), OutScalarT, N>(inputData[i], j);
    }
  }

  return dataOut;
}
}

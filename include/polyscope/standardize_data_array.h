#pragma once

#include "polyscope/messages.h"
#include "polyscope/utilities.h"

#include <complex>
#include <type_traits>
#include <vector>

// This header contains a collection of template functions which enable Polyscope to consume user-defined types, so long
// as they can be accessed by one of several mechanisms.

namespace polyscope {

// == First, define two helper types:

// We'll use this to give precedence to overloads.
template <std::size_t N>
struct PreferenceT : PreferenceT<N - 1> {};
template <>
struct PreferenceT<0> {};

// Used to make static asserts give nice erros
template <typename T>
struct WillBeFalseT : std::false_type {};


// =================================================
// ============ array size adapator
// =================================================

// Adaptor to return the number of elements in an array type
//
// The result is a function `size_t adaptorF_size(const T& inputData)`, which returns the number of elements in the
// array.
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
//   - any user defined function `size_t adaptorF_custom_size(const T& inputData)`
//   - the .size() member function inputData.size()


// Highest priority: any user defined function
template <class T, typename C1 = typename std::enable_if<
                       std::is_same<decltype((size_t)adaptorF_custom_size(*(T*)nullptr)), size_t>::value>::type>
size_t adaptorF_sizeImpl(PreferenceT<2>, const T& inputData) {
  return adaptorF_custom_size(inputData);
}


// Next: call T.size()
template <class T, typename C1 = typename std::enable_if<
                       std::is_same<decltype((size_t)(*(T*)nullptr).size()), size_t>::value>::type>
size_t adaptorF_sizeImpl(PreferenceT<1>, const T& inputData) {
  return inputData.size();
}


// Fall-through case: no overload found :(
template <class T, class S>
size_t adaptorF_sizeImpl(PreferenceT<0>, const T& inputData) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid adaptor for size of array-like data");
  return std::vector<S>();
}


// General version, which will attempt to substitute in to the variants above
template <class T>
size_t adaptorF_size(const T& inputData) {
  return adaptorF_sizeImpl(PreferenceT<2>{}, inputData);
}


// =================================================
// ============ array access adapator
// =================================================

// Adaptor to access elements in an array type.
//
// Suppose the user array type is T, which holds many elements of type S.
// Here, we abstract by writing a function which converts the entire input array to a std::vector<S>.
//
// The result is a function `std::vector<S> adaptorF_convertToStdVector(const T& inputData)`, which is templated on T
// and S.
//
// Note: it might be tempting to instead abstract via a function which which accesses the i'th element of the array,
// but that would require that array types be random-accessable. By going to a std::vector, we open the door to
// non-random-accessible input types.
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
// - user-defined adaptorF_custom_convertToStdVector()
// - bracket access
// - callable (parenthesis) access
// - iterable (begin() and end())


// Highest priority: user-specified function
template <class T, class S,
          typename C1 = typename std::enable_if<
              std::is_same<decltype((S)adaptorF_custom_convertToStdVector(*(T*)nullptr)[0]), S>::value>::type>
std::vector<S> adaptorF_convertToStdVectorImpl(PreferenceT<4>, const T& inputData) {
  auto userVec = adaptorF_custom_convertToStdVector(inputData);
  // If the user-provided function returns something else, try to convert it to a std::vector<S>.
  // (handles e.g. case where user returns std::vector<float> but we want std::vector<double>)
  // In the case where the user function already returns what we want, this costs us an extra copy...
  // maybe one day we can dive back in to template land to remedy.
  std::vector<S> out(userVec.size());
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = userVec[i];
  }
  return out;
}


// Next: any bracket access operator
template <class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr)[(size_t)0]), S>::value>::type>
std::vector<S> adaptorF_convertToStdVectorImpl(PreferenceT<3>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<S> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i] = inputData[i];
  }
  return dataOut;
}

// Next: any callable access operator
template <class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr)((size_t)0)), S>::value>::type>
std::vector<S> adaptorF_convertToStdVectorImpl(PreferenceT<2>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<S> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i] = inputData(i);
  }
  return dataOut;
}

// Next: anything iterable (begin(), end(), etc)
// Note: this test for iterable isn't perfect, might get tricked by something that almost-but-not-quite matches.
template <class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)*std::begin(*(T*)nullptr)), S>::value &&
                                                std::is_same<decltype((S)*std::end(*(T*)nullptr)), S>::value>::type>
std::vector<S> adaptorF_convertToStdVectorImpl(PreferenceT<1>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<S> dataOut(dataSize);
  size_t i = 0;
  for (auto v : inputData) {
    dataOut[i] = v;
    i++;
  }
  return dataOut;
}


// Fall-through case: no overload found :(
template <class T, class S>
std::vector<S> adaptorF_convertToStdVectorImpl(PreferenceT<0>, const T& inputData) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid adaptor for accessing array-like data");
  return std::vector<S>();
}


// General version, which will attempt to substitute in to the variants above
template <class T, class S>
std::vector<S> adaptorF_convertToStdVector(const T& inputData) {
  return adaptorF_convertToStdVectorImpl<T, S>(PreferenceT<4>{}, inputData);
}


// =================================================
// ============ vector access adapator
// =================================================

// =================================================
// ============ array-of-vector access adapator
// =================================================


// =================================================
// ============ standardize functions
// =================================================

// These are utilitiy functions which use the adaptors above to do useful things, like

// Check that a data array has the expected size
template <class T>
void validateSize(const T& inputData, std::vector<size_t> expectedSizes, std::string errorName = "") {

  // No-op if no sizes given
  if (expectedSizes.size() == 0) {
    return;
  }

  size_t dataSize = adaptorF_size(inputData);

  // Simpler error if only one size
  if (expectedSizes.size() == 1) {
    if (dataSize != expectedSizes[0]) {
      error("Size validation failed on data array [" + errorName + "]. Expected size " +
            std::to_string(expectedSizes[0]) + " but has size " + std::to_string(dataSize));
    }
  }
  // General case
  else {

    // Return success if any sizes match
    for (size_t possibleSize : expectedSizes) {
      if (dataSize == possibleSize) {
        return;
      }
    }

    // Build a useful error message
    std::string sizesStr = "{";
    for (size_t possibleSize : expectedSizes) {
      sizesStr += std::to_string(possibleSize) + ",";
    }
    sizesStr += "}";

    error("Size validation failed on data array [" + errorName + "]. Expected size in " + sizesStr + " but has size " +
          std::to_string(dataSize));
  }
}

// Pass through to general version
template <class T>
void validateSize(const T& inputData, size_t expectedSize, std::string errorName = "") {
  validateSize<T>(inputData, std::vector<size_t>{expectedSize}, errorName);
}


// Convert an array of scalar types
// class D: scalar data type
// class T: input array type
// notice: template types here are backwards from the convention in adaptorF_convertToStdVector. This is because it's
// handy to only specify the output type.
template <class D, class T>
std::vector<D> standardizeArray(const T& inputData) {
  return adaptorF_convertToStdVector<T, D>(inputData);
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
      dataOut[i][j] = accessVectorLikeValue<typename std::remove_reference<decltype(inputData[0])>::type, OutScalarT>(
          inputData[i], j);
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

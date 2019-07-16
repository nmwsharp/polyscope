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
// There are several sets of functions below which have identical signatures, except the first argument is a
// PreferenceT<N> for a distinct value of N. Since PreferenceT<N> implicitly converts to PreferenceT<N-1>, a function
// call to f(PreferenceT<BIGGEST_N>, ...) _can_ match any of the variants, but will _prefer_ to match the one with the
// largest N which passes type substitution. This allows us to esacape overload ambiguity, and also explicitly order the
// heirarchy of overloads.
template <std::size_t N>
struct PreferenceT : PreferenceT<N - 1> {};
template <>
struct PreferenceT<0> {};

// Used to make static asserts give nice errors
template <typename T>
struct WillBeFalseT : std::false_type {};

// Get the inner type of a bracket-accessible type
template <typename T>
struct InnerType {
  typedef typename std::remove_reference<decltype((*(T*)nullptr)[0])>::type type;
};


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
//   - the .rows() member function inputData.rows()
//   - the .size() member function inputData.size()


// Highest priority: any user defined function
template <class T, typename C1 = typename std::enable_if<
                       std::is_same<decltype((size_t)adaptorF_custom_size(*(T*)nullptr)), size_t>::value>::type>
size_t adaptorF_sizeImpl(PreferenceT<3>, const T& inputData) {
  return adaptorF_custom_size(inputData);
}

// Next: call T.rows()
template <class T, typename C1 = typename std::enable_if<
                       std::is_same<decltype((size_t)(*(T*)nullptr).rows()), size_t>::value>::type>
size_t adaptorF_sizeImpl(PreferenceT<2>, const T& inputData) {
  return inputData.rows();
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
  return adaptorF_sizeImpl(PreferenceT<3>{}, inputData);
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
// ============ vector-2 access adapator
// =================================================

// Adaptor to access the i'th element of a fixed-sized 2D vector
//
// The result is a function
//   template <class S, unsigned int I, class T>
//   inline S adaptorF_accessVector2Value(const T& inVal);
// which accesses the vector `inVal` at index `I` and returns a scalar of type `S`.
//
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
//   - any user defined function `S adaptorF_custom_accessVector2Value(const T& inputVec, unsigned int ind)`;
//   - bracketed indices T[0] and T[1]
//   - members .x and .y
//   - members .u and .v
//   - members .real() and .imag()


// Highest priority: any user defined function
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<
              std::is_same<decltype((S)adaptorF_custom_accessVector2Value(*(T*)nullptr, 0)), S>::value>::type>
S adaptorF_accessVector2ValueImpl(PreferenceT<5>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  return adaptorF_custom_accessVector2Value(inputVec, I);
}

// Next: bracket indices
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr)[0]), S>::value>::type>
S adaptorF_accessVector2ValueImpl(PreferenceT<4>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  return (S)inputVec[I];
}


// Next: members .x and .y
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr).x), S>::value &&
                                                std::is_same<decltype((S)(*(T*)nullptr).y), S>::value>::type>
S adaptorF_accessVector2ValueImpl(PreferenceT<3>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  if (I == 0) {
    return (S)inputVec.x;
  } else {
    return (S)inputVec.y;
  }
}

// Next: members .u and .v
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr).u), S>::value &&
                                                std::is_same<decltype((S)(*(T*)nullptr).v), S>::value>::type>
S adaptorF_accessVector2ValueImpl(PreferenceT<2>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  if (I == 0) {
    return (S)inputVec.u;
  } else {
    return (S)inputVec.v;
  }
}

// Next: members .real() and .imag()
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr).real()), S>::value &&
                                                std::is_same<decltype((S)(*(T*)nullptr).imag()), S>::value>::type>
S adaptorF_accessVector2ValueImpl(PreferenceT<1>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  if (I == 0) {
    return (S)inputVec.real();
  } else {
    return (S)inputVec.imag();
  }
}

// Fall-through case: no overload found :(
template <unsigned int I, class T, class S>
S adaptorF_accessVector2ValueImpl(PreferenceT<0>, const T& inputVec) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid accessor for 2D vector-like value");
  return S();
}

// General version, which will attempt to substitute in to the variants above
// Templates:
//  - S: output scalar type
//  - I: index at which to access
//  - T: input length-2 vector-like type
template <class S, unsigned int I, class T>
inline S adaptorF_accessVector2Value(const T& inVal) {
  return adaptorF_accessVector2ValueImpl<I, T, S>(PreferenceT<5>{}, inVal);
}


// =================================================
// ============ vector-3 access adapator
// =================================================

// Adaptor to access the i'th element of a fixed-sized 3D vector
//
// The result is a function
//   template <class S, unsigned int I, class T>
//   inline S adaptorF_accessVector3Value(const T& inVal);
// which accesses the vector `inVal` at index `I` and returns a scalar of type `S`.
//
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
//   - any user defined function `S adaptorF_custom_accessVector3Value(const T& inputVec, unsigned int ind)`;
//   - bracketed indices T[0], T[1], T[2]
//   - members .x .y .z


// Highest priority: any user defined function
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<
              std::is_same<decltype((S)adaptorF_custom_accessVector3Value(*(T*)nullptr, 0)), S>::value>::type>
S adaptorF_accessVector3ValueImpl(PreferenceT<3>, const T& inputVec) {
  static_assert(I < 3, "bad vector3 access");
  return adaptorF_custom_accessVector3Value(inputVec, I);
}

// Next: bracket indices
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr)[0]), S>::value>::type>
S adaptorF_accessVector3ValueImpl(PreferenceT<2>, const T& inputVec) {
  static_assert(I < 3, "bad vector3 access");
  return (S)inputVec[I];
}


// Next: members .x .y .z
template <unsigned int I, class T, class S,
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(*(T*)nullptr).x), S>::value &&
                                                std::is_same<decltype((S)(*(T*)nullptr).y), S>::value &&
                                                std::is_same<decltype((S)(*(T*)nullptr).z), S>::value>::type>
S adaptorF_accessVector3ValueImpl(PreferenceT<1>, const T& inputVec) {
  static_assert(I < 3, "bad vector3 access");
  if (I == 0) {
    return (S)inputVec.x;
  } else if (I == 1) {
    return (S)inputVec.y;
  } else {
    return (S)inputVec.z;
  }
}


// Fall-through case: no overload found :(
template <unsigned int I, class T, class S>
S adaptorF_accessVector3ValueImpl(PreferenceT<0>, const T& inputVec) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid accessor for 3D vector-like value");
  return S();
}

// General version, which will attempt to substitute in to the variants above
// Templates:
//  - S: output scalar type
//  - I: index at which to access
//  - T: input length-3 vector-like type
template <class S, unsigned int I, class T>
inline S adaptorF_accessVector3Value(const T& inVal) {
  return adaptorF_accessVector3ValueImpl<I, T, S>(PreferenceT<3>{}, inVal);
}


// =================================================
// ============ array-of-vector access adapator
// =================================================


// Adaptor to convert an array of a dense vectors to a canonical representation. For instance, a list of N vector3's
// (like std::vector<glm::vec3>), or matrix-style representations, like Eigen::MatrixXd<N,3>.
//
// The output is a std::vector<O>, where O is an output type also given as a template argument. The output type must be
// subscriptable up to the inner dimension D, for instance one could use O = glm::vec3 or O = std::array<S,D>.
//
// The result is a function
//   template <class O, unsigned int D, class T>
//   inline std::vector<O> adaptorF_convertArrayOfVectorToStdVector(const T& inputData);
// which converts the input to a std::vector<O>.
//
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
//   - any user defined function
//          std::vector<std::array<F, D>> adaptorF_custom_convertArrayOfVectorToStdVector(const YOUR_TYPE& inputData);
//   - dense callable (parenthesis) access (like T(i,j))
//   - double bracket access (like T[i][j])
//   - bracket xyz vector3 unpacking (like T[0].x)
//   - bracket xy vector2 unpacking (like T[0].x)
//   - bracket uv vector2 unpacking (like T[0].u)
//   - bracket real(),imag() vector2 unpacking (like T[0].real())
//   - iterable bracket (like for(T val : inputData) { val[0] })


// Highest priority: user-specified function

// Note: this dummy function is defined so the non-dependent name adaptorF_custom_convertArrayOfVectorToStdVector will always resolve to something; some compilers will throw an error if the name doesn't resolve. Because this function has variadic arguments, it will always have lower overload priority than a user defined, non variadic overload
template <typename... Types> 
void adaptorF_custom_convertArrayOfVectorToStdVector(Types... args) {
  // dummy function
}

template <class O, unsigned int D, class T,
          typename C1 = typename std::enable_if<std::is_same<
              decltype((typename InnerType<O>::type)(adaptorF_custom_convertArrayOfVectorToStdVector(*(T*)nullptr))[0][0]),
              typename InnerType<O>::type>::value>::type>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<8>, const T& inputData) {

  // should be std::vector<std::array<SCALAR,D>>
  auto userArr = adaptorF_custom_convertArrayOfVectorToStdVector(inputData);

  // This results in an extra copy, which isn't reallllly necessary.

  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = userArr[i][j];
    }
  }
  return dataOut;
}


// Next: any dense callable (parenthesis) access operator
template <class O, unsigned int D, class T,
          typename C1 = typename std::enable_if<
              std::is_same<decltype((typename InnerType<O>::type)(*(T*)nullptr)((size_t)0, (size_t)0)),
                           typename InnerType<O>::type>::value>::type>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<7>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = inputData(i, j);
    }
  }
  return dataOut;
}

// Next: any dense bracket access operator
template <class O, unsigned int D, class T,
          typename C1 = typename std::enable_if<
              std::is_same<decltype((typename InnerType<O>::type)(*(T*)nullptr)[(size_t)0][(size_t)0]),
                           typename InnerType<O>::type>::value>::type>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<6>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = inputData[i][j];
    }
  }
  return dataOut;
}

// Next: bracket-xyz access
template <
    class O, unsigned int D, class T,
    typename C_INNER = typename std::remove_reference<decltype((*(T*)nullptr)[(size_t)0])>::type,
    typename C_RES = typename InnerType<O>::type,
    typename C1 = typename std::enable_if<std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).x), C_RES>::value &&
                                          std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).y), C_RES>::value &&
                                          std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).z), C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<5>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = (C_RES)inputData[i].x;
    dataOut[i][1] = (C_RES)inputData[i].y;
    dataOut[i][2] = (C_RES)inputData[i].z;
  }
  return dataOut;
}

// Next: bracket-xy access
template <
    class O, unsigned int D, class T,
    typename C_INNER = typename std::remove_reference<decltype((*(T*)nullptr)[(size_t)0])>::type,
    typename C_RES = typename InnerType<O>::type,
    typename C1 = typename std::enable_if<std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).x), C_RES>::value &&
                                          std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).y), C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<4>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = (C_RES)inputData[i].x;
    dataOut[i][1] = (C_RES)inputData[i].y;
  }
  return dataOut;
}

// Next: bracket-uv access
template <
    class O, unsigned int D, class T,
    typename C_INNER = typename std::remove_reference<decltype((*(T*)nullptr)[(size_t)0])>::type,
    typename C_RES = typename InnerType<O>::type,
    typename C1 = typename std::enable_if<std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).u), C_RES>::value &&
                                          std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).v), C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<3>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = (C_RES)inputData[i].u;
    dataOut[i][1] = (C_RES)inputData[i].v;
  }
  return dataOut;
}


// Next: bracket-real/imag access
template <class O, unsigned int D, class T,
          typename C_INNER = typename std::remove_reference<decltype((*(T*)nullptr)[(size_t)0])>::type,
          typename C_RES = typename InnerType<O>::type,
          typename C1 =
              typename std::enable_if<std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).real()), C_RES>::value &&
                                      std::is_same<decltype((C_RES)(*(C_INNER*)nullptr).imag()), C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<2>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = (C_RES)inputData[i].real();
    dataOut[i][1] = (C_RES)inputData[i].imag();
  }
  return dataOut;
}


// Next: iterable (begin(), end(), etc) - bracket accesss
// Note: this test for iterable isn't perfect, might get tricked by something that almost-but-not-quite matches.
template <class O, unsigned int D, class T, typename C_RES = typename InnerType<O>::type,
          typename C1 =
              typename std::enable_if<std::is_same<decltype((C_RES)(*std::begin(*(T*)nullptr))[0]), C_RES>::value &&
                                      std::is_same<decltype((C_RES)(*std::end(*(T*)nullptr))[0]), C_RES>::value>::type>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<1>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  size_t i = 0;
  for (auto v : inputData) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = v[j];
    }
    i++;
  }
  return dataOut;
}


// Fall-through case: no overload found :(
// template <class O, unsigned int D, class T>
// std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<0>, const T& inputData) {
// static_assert(WillBeFalseT<T>::value,
//"could not resolve valid adaptor for accessing array-of-vectors-like input data");
// return std::vector<O>();
//}


// General version, which will attempt to substitute in to the variants above
template <class O, unsigned int D, class T>
std::vector<O> adaptorF_convertArrayOfVectorToStdVector(const T& inputData) {
  return adaptorF_convertArrayOfVectorToStdVectorImpl<O, D, T>(PreferenceT<8>{}, inputData);
}


// =================================================
// ============ nested array access adapator
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
// notice: template types here are backwards from the convention in adaptorF_convertToStdVector. This is
// because it's handy to only specify the output type.
template <class D, class T>
std::vector<D> standardizeArray(const T& inputData) {
  return adaptorF_convertToStdVector<T, D>(inputData);
}


// Convert an array of vector types
// class T: input array type
// notice: template types here are backwards from the convention in adaptorF_convertToStdVector. This is
// because it's handy to only specify the output type.
template <class O, unsigned int D, class T>
std::vector<O> standardizeVectorArray(const T& inputData) {
  return adaptorF_convertArrayOfVectorToStdVector<O, D, T>(inputData);
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

// Convert an array of low-dimensional vector types. Outer type must support size() method. Inner type
// dimensions are not checked, and must match expected inner dimension N.
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

// Convert a nested array where the inner types have variable length. Inner and outer types must support
// a size() method. Always returns a std::vector<std::vector<D>>
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

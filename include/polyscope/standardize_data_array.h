// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/messages.h"
#include "polyscope/utilities.h"

#include <type_traits>
#include <vector>

// This header contains a collection of template functions which enable Polyscope to consume user-defined types, so long
// as they can be accessed by one of several mechanisms.

// clang-format off

// == "How do these work", 
// Quick guide assuming only a basic knowledge of C++ & templates
// Nicholas Sharp (nsharp@cs.cmu.edu)
//
// These templates use a technique called SFINAE, which abuses C++'s function 
// resolution rules to consider many version of a function, and pick the one 
// which will compile. We use technique to heuristically convert unknown user 
// types to a standard format.
//
// This isn't normally something the language supports, but technically you can 
// do it, thanks to a "feature" called SFINAE (Substitution Failure Is Not An 
// Error). SFINAE is based on a quirk of how function calls get resolved by 
// the compiler:
//
//   (1) The compiler assembles a list of all available functions whose names
//       match a given function call.
//
//   (2) The compiler attempts to substitute types in to the function signatures 
//       (aka templates, parameters, and return types) from (1). If any of these
//       substitutions fail, that function is discarded, but processing
//       continues (this is SFINAE---surprisingly, it's not a compiler error).
//
//   (3) 
//        (a) If no valid function overloads remain, an error is thrown, no
//            function exists.
//        (b) If just one overload remains, it is used.
//        (c) If multiple valid overloads remain, they are ranked, according
//            to a few rules like preferring not to use variadic functions,
//            and preferring functions that do not require implicit conversion
//            of parameters over those that do. If there is a unique highest-
//            ranked function, it is used. Otherwise, an error is issued about
//            ambiguous function calls.
//
// We utilize SFINAE by ensuring that versions of a function which are not
// applicable get discarded during step (2). There are a few ways to do this,
// but we'll do it by adding extra template types with default values that
// fail to resolve. 
//
//   Here's a quick example to get the idea across. Suppose we have a 
//   function `template<class T> f(T x)`, which will invoke `x.doStuff()`. 
//
//   We can add an extra template argument to f() to make sure 
//   x.doStuff() exists.
//
//     template<typename T, 
//          typename C1 = decltype(std::declval<T>().doStuff())>
//     void f(T x) { /* function body */ }
// 
//   unpacking this:
//      - decltype() yields the type that the expression in its interior
//        would return if it were evaluated (but it won't be evaluated).
//      - The expression std::declval<T>() results in an object of type T. 
//        It's not actual constructor, it would be illegal to use 
//        the expression in code that actually gets executed. But that's 
//        fine, we will just use it in template expressions to get objects 
//        of type T as part of the template logic.
//      - We then invoke .doStuff() on the T we just created; so the
//        decltype() it's wrapped in can output the return type of 
//        doStuff().
//   
//   If T does have a doStuff() method, C1 will simply hold the return type
//   of doStuff(). However, if it does not, the expression will be rejected,
//   and this whole function will be rejected, so another candidate can be
//   considered. And that's our core trick!
//
// There are a few other mechanisms we will make use of below that we
// should briefly mention:
//
//
// [std::is_same, std::enable_if] Sometimes we don't just need to check 
// if a function exists, we also care about what it returns. Two helper
// templates from the stl can be used to get us there.
//
//    std::is_same<U,V> has a constant member ::value, which is true if 
//    the two template arguments are the same type, and false otherwise.
//
//    std::enable_if<B> has a member type ::type which is defined if 
//    the boolean condition B is true, and does not exist otherwise.
//    (recall, if ::type doesn't exist, it'll halt substitution as
//    we discussed in (2) above).
//
//  Putting these two together we can require that a function x.doStuff()
//  return an int with a recipe like
//
//     template<typename T, 
//          typename C1 = typename std::enable_if<
//            std::is_same<decltype(std::declval<T>().doStuff()), int>::value
//            >::type>
//     void f(T x) { /* function body */ }
//
//
// [PreferenceT<>] Case (3a) from the list above represents a problem for
// us. What if more than one of our candidate functions is valid, and 
// passes the template checks? This would lead to errors about ambiguous
// function overloads. We need to make sure that when multiple functions 
// are valid, one is always preferred over all others.
//
// Our mechanism to do so will be including an extra dummy parameter 
// PreferenceT<N> in every function's argument list, with a distinct 
// value of N. Our Preference<N> is implicitly convertible to 
// PreferenceT<N-1> (and so on). So if our initial function call uses 
// PreferenceT<N_MAX>{}, it will match any of the overloads with lower-
// numbered PreferenceT<> parameters. However, since these lower-numbered
// functions involve implicit conversions, they will always have a lower
// priority and not be ambiguous!


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
  typedef typename std::remove_reference<decltype(std::declval<T>()[0])>::type type;
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


// Note: this dummy function is defined so the non-dependent user function name will always resolve to something; 
// some compilers will throw an error if the name doesn't resolve. 
inline void adaptorF_custom_size(void* dont_use) {
  // dummy function
}

// Highest priority: any user defined function
template <class T, 
  /* condition: user function exists and returns something that can be cast to size_t */       
  typename C1 = typename std::enable_if< std::is_same<decltype((size_t)adaptorF_custom_size(std::declval<T>())), size_t>::value>::type>

size_t adaptorF_sizeImpl(PreferenceT<3>, const T& inputData) {
  return adaptorF_custom_size(inputData);
}

// Next: call T.rows()
template <class T, 
  /* condition: has .rows() method which returns something that can be cast to size_t */
  typename C1 = typename std::enable_if<std::is_same<decltype((size_t)(std::declval<T>()).rows()), size_t>::value>::type>

size_t adaptorF_sizeImpl(PreferenceT<2>, const T& inputData) {
  return inputData.rows();
}


// Next: call T.size()
template <class T, 
  /* condition: has .size() method which returns something that can be cast to size_t */
  typename C1 = typename std::enable_if< std::is_same<decltype((size_t)(std::declval<T>()).size()), size_t>::value>::type>

size_t adaptorF_sizeImpl(PreferenceT<1>, const T& inputData) {
  return inputData.size();
}


// Fall-through case: no overload found :(
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <class T, class S>
size_t adaptorF_sizeImpl(PreferenceT<0>, const T& inputData) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid adaptor for size of array-like data");
  return std::vector<S>();
}
#endif

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
// but that would require that array types be random-accessible. By going to a std::vector, we open the door to
// non-random-accessible input types like iterables.
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
// - user-defined adaptorF_custom_convertToStdVector()
// - bracket access
// - callable (parenthesis) access
// - iterable (begin() and end())


// Note: this dummy function is defined so the non-dependent user function name will always resolve to something; 
// some compilers will throw an error if the name doesn't resolve. 
inline void adaptorF_custom_convertToStdVector(void* dont_use) {
  // dummy function
}

// Highest priority: user-specified function
template <class T, class S,
  /* condition: user defined function exists and returns something that can be bracket-indexed to get an S */
  typename C1 = typename std::enable_if< std::is_same<decltype((S)adaptorF_custom_convertToStdVector(std::declval<T>())[0]), S>::value>::type>

void adaptorF_convertToStdVectorImpl(PreferenceT<4>, const T& inputData, std::vector<S>& out) {
  auto userVec = adaptorF_custom_convertToStdVector(inputData);

  // If the user-provided function returns something else, try to convert it to a std::vector<S>.
  // (handles e.g. case where user returns std::vector<float> but we want std::vector<double>)
  // In the case where the user function already returns what we want, this costs us an extra copy...
  // maybe one day we can dive back in to template land to remedy.
  
  out.resize(userVec.size());

  for (size_t i = 0; i < out.size(); i++) {
    out[i] = userVec[i];
  }
}


// Next: any bracket access operator
template <class T, class S,
  /* condition: input can be bracket-indexed to get an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>())[(size_t)0]), S>::value>::type>

void adaptorF_convertToStdVectorImpl(PreferenceT<3>, const T& inputData, std::vector<S>& dataOut) {
  size_t dataSize = adaptorF_size(inputData);
  dataOut.resize(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i] = inputData[i];
  }
}


// Next: any callable access operator
template <class T, class S,
  /* condition: input can be called (aka parenthesis-indexed) to get an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>())((size_t)0)), S>::value>::type>

void adaptorF_convertToStdVectorImpl(PreferenceT<2>, const T& inputData, std::vector<S>& dataOut) {
  size_t dataSize = adaptorF_size(inputData);
  dataOut.resize(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i] = inputData(i);
  }
}


// Next: anything iterable (begin(), end(), etc)
// Note: this test for iterable isn't perfect, might get tricked by something that almost-but-not-quite matches.
template <class T, class S,
  /* condition: input has a begin() and end() function, both of which can be dereferenced to get an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)*std::begin(std::declval<T>())), S>::value &&
                                        std::is_same<decltype((S)*std::end(std::declval<T>())), S>::value>::type>

void adaptorF_convertToStdVectorImpl(PreferenceT<1>, const T& inputData, std::vector<S>& dataOut) {
  size_t dataSize = adaptorF_size(inputData);
  dataOut.resize(dataSize);
  size_t i = 0;
  for (auto v : inputData) {
    dataOut[i] = v;
    i++;
  }
}


// Fall-through case: no overload found :(
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <class T, class S>
void adaptorF_convertToStdVectorImpl(PreferenceT<0>, const T& inputData, std::vector<S>& dataOut) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid adaptor for accessing array-like data");
}
#endif


// General version, which will attempt to substitute in to the variants above
template <class S, class T>
void adaptorF_convertToStdVector(const T& inputData, std::vector<S>& dataOut) {
  adaptorF_convertToStdVectorImpl<T, S>(PreferenceT<4>{}, inputData, dataOut);
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


// Note: this dummy function is defined so the non-dependent user function name will always resolve to something; 
// some compilers will throw an error if the name doesn't resolve. 
inline void adaptorF_custom_accessVector2Value(void* dont_use) {
  // dummy function
}

// Highest priority: any user defined function
template <unsigned int I, class T, class S,
  /* condition: user function exists and retuns something that can be cast to an S */
  typename C1 = typename std::enable_if< std::is_same<decltype((S)adaptorF_custom_accessVector2Value(std::declval<T>(), 0)), S>::value>::type>

S adaptorF_accessVector2ValueImpl(PreferenceT<5>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  return adaptorF_custom_accessVector2Value(inputVec, I);
}


// Next: bracket indices
template <unsigned int I, class T, class S,
  /* condition: input can be bracket-index to get something that can be cast to an S */
          typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>())[0]), S>::value>::type>

S adaptorF_accessVector2ValueImpl(PreferenceT<4>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  return (S)inputVec[I];
}


// Next: members .x and .y
template <unsigned int I, class T, class S,
  /* condition: input has .x and .y members which give something that can be cast to an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>()).x), S>::value &&
                                        std::is_same<decltype((S)(std::declval<T>()).y), S>::value>::type>

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
  /* condition: input has .u and .v members which give something that can be cast to an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>()).u), S>::value &&
                                        std::is_same<decltype((S)(std::declval<T>()).v), S>::value>::type>

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
  /* condition: input has .real() and .imag() member functions which give something that can be cast to an S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>()).real()), S>::value &&
                                        std::is_same<decltype((S)(std::declval<T>()).imag()), S>::value>::type>

S adaptorF_accessVector2ValueImpl(PreferenceT<1>, const T& inputVec) {
  static_assert(I < 2, "bad vector2 access");
  if (I == 0) {
    return (S)inputVec.real();
  } else {
    return (S)inputVec.imag();
  }
}

// Fall-through case: no overload found :(
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <unsigned int I, class T, class S>
S adaptorF_accessVector2ValueImpl(PreferenceT<0>, const T& inputVec) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid accessor for 2D vector-like value");
  return S();
}
#endif

// General version, which will attempt to substitute in to the variants above
// Templates:
//  - S: output scalar type
//  - I: index at which to access
//  - T: input length-2 vector-like type
template <class S, unsigned int I, class T,
    /* condition: I must be < 2 */
    class C1 = typename std::enable_if< (I < 2) >::type>
S adaptorF_accessVector2Value(const T& inVal) {
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


// Note: this dummy function is defined so the non-dependent user function name will always resolve to something; 
// some compilers will throw an error if the name doesn't resolve.
inline void adaptorF_custom_accessVector3Value(void* dont_use) {
  // dummy function
}

// Highest priority: any user defined function
template <unsigned int I, class T, class S,
  /* condition: user function exists and returns something that can be cast to S */
  typename C1 = typename std::enable_if< std::is_same<decltype((S)adaptorF_custom_accessVector3Value(std::declval<T>(), 0)), S>::value>::type>

S adaptorF_accessVector3ValueImpl(PreferenceT<3>, const T& inputVec) {
  static_assert(I < 3, "bad vector3 access");
  return adaptorF_custom_accessVector3Value(inputVec, I);
}


// Next: bracket indices
template <unsigned int I, class T, class S,
  /* condition: input can be bracket-indexed to get something that can be cast to S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>())[0]), S>::value>::type>

S adaptorF_accessVector3ValueImpl(PreferenceT<2>, const T& inputVec) {
  static_assert(I < 3, "bad vector3 access");
  return (S)inputVec[I];
}


// Next: members .x .y .z
template <unsigned int I, class T, class S,
  /* condition: input has .x .y .z members which hold something that can be cast to S */
  typename C1 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>()).x), S>::value &&
                                        std::is_same<decltype((S)(std::declval<T>()).y), S>::value &&
                                        std::is_same<decltype((S)(std::declval<T>()).z), S>::value>::type>

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
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <unsigned int I, class T, class S>
S adaptorF_accessVector3ValueImpl(PreferenceT<0>, const T& inputVec) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid accessor for 3D vector-like value");
  return S();
}
#endif

// General version, which will attempt to substitute in to the variants above
// Templates:
//  - S: output scalar type
//  - I: index at which to access
//  - T: input length-3 vector-like type
template <class S, unsigned int I, class T,
    /* condition: I must be < 3 */
    class C1 = typename std::enable_if< (I < 3) >::type>

S adaptorF_accessVector3Value(const T& inVal) {
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
//   - outer type bracket accessbile, inner anything convertible to Vector2/3
//   - outer type iteratble, inner type anything convertible to Vector2/3
//   - iterable bracket (like for(T val : inputData) { val[0] })


// Highest priority: user-specified function

// Note: this dummy function is defined so the non-dependent user function name will always resolve to something; 
// some compilers will throw an error if the name doesn't resolve.
inline void adaptorF_custom_convertArrayOfVectorToStdVector(void* dont_use) {
  // dummy function
}

template <
    class O, unsigned int D, class T,
    /* condition: user function exists and returns something that can be bracket-indexed to get an S */
    typename C1 = typename std::enable_if<std::is_same< 
                                          decltype((typename InnerType<O>::type)(adaptorF_custom_convertArrayOfVectorToStdVector(std::declval<T>()))[0][0]), 
                                          typename InnerType<O>::type>::value>::type>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<9>, const T& inputData) {

  // should be std::vector<std::array<SCALAR,D>>
  auto userArr = adaptorF_custom_convertArrayOfVectorToStdVector(inputData);

  // This results in an extra copy, which isn't reallllly necessary.

  size_t dataSize = userArr.size();
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = userArr[i][j];
    }
  }
  return dataOut;
}

// Next: tuple {data_ptr, size} (size is number of vector entries, so ptr should point to D*size valid entries)
template <class O, unsigned int D, class T,
    /* condition: first entry of input can be dereferenced to get a type castable to the scalar type O */
    typename C_DATA = decltype(static_cast<typename InnerType<O>::type>(*std::get<0>(std::declval<T>()))),
    /* condition: second entry of input is castable to an index type */
    typename C_COUNT = decltype(static_cast<size_t>(std::get<1>(std::declval<T>())))
  >

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<8>, const T& inputData) {

  size_t dataSize = static_cast<size_t>(std::get<1>(inputData));
  auto* dataPtr = std::get<0>(inputData);

  std::vector<O> dataOut(dataSize);

  for (size_t i = 0; i < dataSize; i++) {
    for (size_t j = 0; j < D; j++) {
      dataOut[i][j] = dataPtr[D * i + j];
    }
  }

  return dataOut;
}


// Next: any dense callable (parenthesis) access operator
template <class O, unsigned int D, class T,
    /* condition: input can be called with two integer arguments to get something that can be cast to the inner type of O */
    typename C1 = typename std::enable_if<std::is_same<
                                          decltype((typename InnerType<O>::type)(std::declval<T>())((size_t)0, (size_t)0)),
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
    /* condition: input can be bracket-indexed twice to get something that can be cast to the inner type of O */
    typename C1 = typename std::enable_if<std::is_same<
                                          decltype((typename InnerType<O>::type)(std::declval<T>())[(size_t)0][(size_t)0]),
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


// Next: bracketed array of anything adaptable to vector3
template <class O, unsigned int D, class T,
    /* helper type: inner type that results from bracket-indexing T */
    typename C_INNER = typename std::remove_reference<decltype((std::declval<T>())[(size_t)0])>::type,
    /* helper type: inner type of output O */
    typename C_RES = typename InnerType<O>::type,
    /* helper type: scalar type that results from a vector3 access on C_INNER */
    typename C_INNER_SCALAR = decltype(adaptorF_accessVector3Value<C_RES, 0>((std::declval<C_INNER>()))),
    /* condition: output dimension must be 3 */
    typename C1 = typename std::enable_if<D == 3>::type,
    /* condition: the inner_scalar that comes from the vector3 unpack must match the requested inner type */
    typename C2 = typename std::enable_if<std::is_same<C_INNER_SCALAR, C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<5>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = adaptorF_accessVector3Value<C_RES, 0>(inputData[i]);
    dataOut[i][1] = adaptorF_accessVector3Value<C_RES, 1>(inputData[i]);
    dataOut[i][2] = adaptorF_accessVector3Value<C_RES, 2>(inputData[i]);
  }
  return dataOut;
}

// Next: bracketed array of anything adaptable to vector2
template <class O, unsigned int D, class T,
    /* helper type: inner type that results from bracket-indexing T */
    typename C_INNER = typename std::remove_reference<decltype((std::declval<T>())[(size_t)0])>::type,
    /* helper type: inner type of output O */
    typename C_RES = typename InnerType<O>::type,
    /* helper type: scalar type that results from a vector2 access on C_INNER */
    typename C_INNER_SCALAR = decltype(adaptorF_accessVector2Value<C_RES, 0>((std::declval<C_INNER>()))),
    /* condition: output dimension must be 2 */
    typename C1 = typename std::enable_if<D == 2>::type,
    /* condition: the inner_scalar that comes from the vector2 unpack must match the requested inner type */
    typename C2 = typename std::enable_if<std::is_same<C_INNER_SCALAR, C_RES>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<4>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  for (size_t i = 0; i < dataSize; i++) {
    dataOut[i][0] = adaptorF_accessVector2Value<C_RES, 0>(inputData[i]);
    dataOut[i][1] = adaptorF_accessVector2Value<C_RES, 1>(inputData[i]);
  }
  return dataOut;
}


// Next: iterable array of anything adaptable to vector3
template <class O, unsigned int D, class T,
    /* helper type: inner type that results from dereferencing begin() */
    typename C_INNER = typename std::remove_reference<decltype(*(std::declval<T>()).begin())>::type,
    /* helper type: inner type that results from dereferencing end() */
    typename C_INNER_END = typename std::remove_reference<decltype(*(std::declval<T>()).end())>::type,
    /* helper type: inner type of output O */
    typename C_RES = typename InnerType<O>::type,
    /* helper type: scalar type that results from a vector3 access on C_INNER */
    typename C_INNER_SCALAR = decltype(adaptorF_accessVector3Value<C_RES, 0>((std::declval<C_INNER>()))),
    /* condition: output dimension must be 3 */
    typename C1 = typename std::enable_if<D == 3>::type,
    /* condition: the inner_scalar that comes from the vector3 unpack must match the requested inner type */
    typename C2 = typename std::enable_if<std::is_same<C_INNER_SCALAR, C_RES>::value>::type,
    /* condition: the type that comes from begin() must match the one from end() */
    typename C3 = typename std::enable_if<std::is_same<C_INNER, C_INNER_END>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<3>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  size_t i = 0;
  for (auto v : inputData) {
    dataOut[i][0] = adaptorF_accessVector3Value<C_RES, 0>(v);
    dataOut[i][1] = adaptorF_accessVector3Value<C_RES, 1>(v);
    dataOut[i][2] = adaptorF_accessVector3Value<C_RES, 2>(v);
    i++;
  }
  return dataOut;
}

// Next: iterable array of anything adaptable to vector2
template <class O, unsigned int D, class T,
    /* helper type: inner type that results from dereferencing begin() */
    typename C_INNER = typename std::remove_reference<decltype(*(std::declval<T>()).begin())>::type,
    /* helper type: inner type that results from dereferencing end() */
    typename C_INNER_END = typename std::remove_reference<decltype(*(std::declval<T>()).end())>::type,
    /* helper type: inner type of output O */
    typename C_RES = typename InnerType<O>::type,
    /* helper type: scalar type that results from a vector2 access on C_INNER */
    typename C_INNER_SCALAR = decltype(adaptorF_accessVector2Value<C_RES, 0>((std::declval<C_INNER>()))),
    /* condition: output dimension must be 2 */
    typename C1 = typename std::enable_if<D == 2>::type,
    /* condition: the inner_scalar that comes from the vector2 unpack must match the requested inner type */
    typename C2 = typename std::enable_if<std::is_same<C_INNER_SCALAR, C_RES>::value>::type,
    /* condition: the type that comes from begin() must match the one from end() */
    typename C3 = typename std::enable_if<std::is_same<C_INNER, C_INNER_END>::value>::type>

std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<2>, const T& inputData) {
  size_t dataSize = adaptorF_size(inputData);
  std::vector<O> dataOut(dataSize);
  size_t i = 0;
  for (auto v : inputData) {
    dataOut[i][0] = adaptorF_accessVector2Value<C_RES, 0>(v);
    dataOut[i][1] = adaptorF_accessVector2Value<C_RES, 1>(v);
    i++;
  }
  return dataOut;
}


// Next: iterable (begin(), end(), etc) + bracket accesss
// Note: this test for iterable isn't perfect, might get tricked by something that almost-but-not-quite matches.
template <class O, unsigned int D, class T, typename C_RES = typename InnerType<O>::type,
    /* condition: begin() and end() should return something bracket-indexable to yield the inner type of O  */
    typename C1 = typename std::enable_if<std::is_same<decltype((C_RES)(*std::begin(std::declval<T>()))[0]), C_RES>::value &&
                                          std::is_same<decltype((C_RES)(*std::end(std::declval<T>()))[0]), C_RES>::value>::type>

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
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <class O, unsigned int D, class T>
std::vector<O> adaptorF_convertArrayOfVectorToStdVectorImpl(PreferenceT<0>, const T& inputData) {
  static_assert(WillBeFalseT<T>::value,
                "could not resolve valid adaptor for accessing array-of-vectors-like input data");
  return std::vector<O>();
}
#endif


// General version, which will attempt to substitute in to the variants above
template <class O, unsigned int D, class T>
std::vector<O> adaptorF_convertArrayOfVectorToStdVector(const T& inputData) {
  return adaptorF_convertArrayOfVectorToStdVectorImpl<O, D, T>(PreferenceT<9>{}, inputData);
}


// =================================================
// ============ nested array access adapator
// =================================================

// Adaptor to convert an array of arrays to a canonical representation. Here, the array can be "ragged"--not all of
// the inner arrays need to have the same length (though they certainly may). Possible inputs might be a
// `std::vector<std::vector<size_t>>`, or an `Eigen::MatrixXd`.
//
// The output is a std::vector<std::vector<S>>, where S is an output scalar type also given as a template argument.
//
// The result is a function
//   template <class S, class T>
//   inline std::vector<std::vector<S>> adaptorF_convertNestedArrayToStdVector(const T& inputData);
//
//
// The following hierarchy of strategies will be attempted, with decreasing precedence:
//   - any user defined function
//          std::vector<std::vector<S>> adaptorF_custom_convertNestedArrayToStdVector(const YOUR_TYPE& inputData);
//   - dense callable (parenthesis) access (like T(i,j)), on a type that supports .rows() and .cols()
//   - recursive unpacking with bracket
//   - recursive unpacking with parent
//   - recursive unpacking with iterable


// Note: this dummy function is defined so the non-dependent name adaptorF_custom_convertArrayOfVectorToStdVector will
// always resolve to something; some compilers will throw an error if the name doesn't resolve.
inline void adaptorF_custom_convertNestedArrayToStdVector(void* dont_use) {
  // dummy function
}

// Highest priority: user-specified function
template <class S, class I, class T,
    /* condition: user function must be return a tuple of vectors with the compatible type (techincally this just checks for bracket-indexible-thing */
    typename C1 = typename std::enable_if<std::is_same<decltype((S)std::get<0>(adaptorF_custom_convertNestedArrayToStdVector(std::declval<T>()))[0]), S>::value>::type,
    typename C2 = typename std::enable_if<std::is_same<decltype((I)std::get<1>(adaptorF_custom_convertNestedArrayToStdVector(std::declval<T>()))[0]), I>::value>::type
  >

std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<6>, const T& inputData) {

  // should be std::tuple<std::vector<S>, std::vector<I>>
  auto userArrTuple = adaptorF_custom_convertNestedArrayToStdVector(inputData);
  auto userDataArr = std::get<0>(userArrTuple);
  auto userDataStartArr = std::get<1>(userArrTuple);

  // TODO: question for past Nick, why do we do this copy for every custom function?

  // create output tuples
  size_t dataSize = userDataArr.size();
  size_t dataStartSize = userDataStartArr.size();
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);
  dataOut.resize(dataSize);
  dataStartOut.resize(dataStartSize);

  // copy data over
  for (size_t i = 0; i < dataSize; i++) dataOut[i] = userDataArr[i];
  for (size_t i = 0; i < dataStartSize; i++) dataStartOut[i] = userDataStartArr[i];
  return outTuple;
}

// Next: tuple {data_ptr, outer_size, inner_size}
// This is LIMITED to rectangular data only
// A Fx3 array would be passed as {ptr, F, 3}
template <class S, class I, class T,
    /* condition: first entry of input can be dereferenced to get a type castable to the scalar type O */
    typename C_DATA = decltype(static_cast<S>(*std::get<0>(std::declval<T>()))),
    /* condition: second & third entry of input is castable to an index type */
    typename C_OUTER_COUNT = decltype(static_cast<size_t>(std::get<1>(std::declval<T>()))),
    typename C_INNER_COUNT = decltype(static_cast<size_t>(std::get<2>(std::declval<T>())))
  >

std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<5>, const T& inputData) {

  auto* dataPtr = std::get<0>(inputData);
  size_t outerSize = static_cast<size_t>(std::get<1>(inputData));
  size_t innerSize = static_cast<size_t>(std::get<2>(inputData));
  
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);

  dataOut.resize(outerSize * innerSize);
  dataStartOut.resize(outerSize+1);
  
  dataStartOut[0] = 0;

  for (size_t i = 0; i < outerSize * innerSize; i++) {
    dataOut[i] = dataPtr[i];
  }
  for (size_t i = 1; i <= outerSize; i++) {
      dataStartOut[i] = i * innerSize;
  }

  return outTuple;
}


// Next: any dense callable (parenthesis) access operator
template <class S, class I, class T,
    /* condition: must have .rows() function which return something like size_t */
    typename C1 = typename std::enable_if<std::is_same<decltype((size_t)(std::declval<T>()).rows()), size_t>::value>::type,
    /* condition: must have .cols() function which return something like size_t */
    typename C2 = typename std::enable_if<std::is_same<decltype((size_t)(std::declval<T>()).cols()), size_t>::value>::type,
    /* condition: must have be able to call with two size_t arguments to get something that can be cast to S */
    typename C3 = typename std::enable_if<std::is_same<decltype((S)(std::declval<T>())((size_t)0, (size_t)0)), S>::value>::type>

std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<4>, const T& inputData) {

  size_t outerSize = (size_t)inputData.rows();
  size_t innerSize = (size_t)inputData.cols();
  
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);
  dataOut.resize(outerSize*innerSize);
  dataStartOut.resize(outerSize+1);

  dataStartOut[0] = 0;

  for (size_t i = 0; i < outerSize; i++) {
    for (size_t j = 0; j < innerSize; j++) {
      dataOut[innerSize * i + j] = inputData(i, j);
    }
    dataStartOut[i+1] = innerSize * (i + 1);
  }

  return outTuple;
}


// Next: recusive unpacking with bracket
template <class S, class I, class T,
    /* helper type: the result of bracket access on the outer type */
    typename T_INNER = typename std::remove_reference<decltype((std::declval<T>())[0])>::type,
    /* condition: ensure that calling the inner array adaptor works */
    typename C1 = decltype(adaptorF_convertToStdVector<S>(std::declval<T_INNER>(), std::declval<std::vector<S>&>()))
  >
std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<3>, const T& inputData) {

  size_t outerSize = adaptorF_size(inputData);
  
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);
  dataStartOut.resize(outerSize+1);

  std::vector<S> tempVec;

  for (size_t i = 0; i < outerSize; i++) {
    adaptorF_convertToStdVector<S>(inputData[i], tempVec);
    for(auto& s : tempVec) {
      dataOut.push_back(s);
    }
    dataStartOut[i+1] = dataOut.size();
  }

  return outTuple;
}

// Next: recusive unpacking with paren
template <class S, class I, class T,
    /* helper type: the result of paren access on the outer type */
    typename T_INNER = typename std::remove_reference<decltype((std::declval<T>())(0))>::type,
    /* condition: ensure that calling the inner array adaptor works */
    typename C1 = decltype(adaptorF_convertToStdVector<S>(std::declval<T_INNER>(), std::declval<std::vector<S>&>()))
  >

std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<2>, const T& inputData) {

  size_t outerSize = adaptorF_size(inputData);
  
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);
  dataStartOut.resize(outerSize+1);
  
  std::vector<S> tempVec;

  for (size_t i = 0; i < outerSize; i++) {
    adaptorF_convertToStdVector<S>(inputData(i), tempVec);
    for(auto& s : tempVec) {
      dataOut.push_back(s);
    }
    dataStartOut[i+1] = dataOut.size();
  }

  return outTuple;
}


// Next: recusive unpacking with iterable
template <class S, class I, class T,
    /* helper type: the result of dereferencing begin() on the outer type */
    typename T_INNER = typename std::remove_reference<decltype(*(std::declval<T>()).begin())>::type,
    /* helper type: the result of dereferencing end() on the outer type */
    typename T_INNER_END = typename std::remove_reference<decltype(*(std::declval<T>()).end())>::type,
    /* condition: ensure that calling the inner array adaptor works */
    typename C1 = decltype(adaptorF_convertToStdVector<S>(std::declval<T_INNER>(), std::declval<std::vector<S>&>())),
    /* condition: T_INNER must match T_INNER_END */
    typename C2 = typename std::enable_if< std::is_same<T_INNER, T_INNER_END>::value>::type
  >

std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVectorImpl(PreferenceT<1>, const T& inputData) {

  size_t outerSize = adaptorF_size(inputData);
  
  std::tuple<std::vector<S>, std::vector<I>> outTuple;
  std::vector<S>& dataOut = std::get<0>(outTuple);
  std::vector<I>& dataStartOut = std::get<1>(outTuple);
  dataStartOut.resize(outerSize+1);

  std::vector<S> tempVec;

  size_t i = 0;
  for (const auto& n : inputData) {
    adaptorF_convertToStdVector<S>(n, tempVec);
    for(auto& s : tempVec) {
      dataOut.push_back(s);
    }
    dataStartOut[i+1] = dataOut.size();
    i++;
  }

  return outTuple;
}

// Fall-through case: no overload found :(
// We use this to print a slightly less scary error message.
#ifndef POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH
template <class S, class I, class T>
std::tuple<std::vector<S>, std::vector<I>>
adaptorF_convertNestedArrayToStdVector(PreferenceT<0>, const T& inputData) {
  static_assert(WillBeFalseT<T>::value, "could not resolve valid adaptor for accessing nested-array-like input data");
  return std::tuple<std::vector<S>, std::vector<I>>();
}
#endif


// General version, which will attempt to substitute in to the variants above
template <class S, class I, class T>
std::tuple<std::vector<S>, std::vector<I>> adaptorF_convertNestedArrayToStdVector(const T& inputData) {
  return adaptorF_convertNestedArrayToStdVectorImpl<S, I, T>(PreferenceT<6>{}, inputData);
}

// clang-format on

// =================================================
// ============ standardize functions
// =================================================

// These are utilitiy functions which use the adaptors above to do useful things, with nicer syntax.

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
      exception("Size validation failed on data array [" + errorName + "]. Expected size " +
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

    exception("Size validation failed on data array [" + errorName + "]. Expected size in " + sizesStr +
              " but has size " + std::to_string(dataSize));
  }
}

// Pass through to general version which takes a single expected size
template <class T>
void validateSize(const T& inputData, size_t expectedSize, std::string errorName = "") {
  validateSize<T>(inputData, std::vector<size_t>{expectedSize}, errorName);
}

// Convert a single fixed-size 2D vector
// class O: output vector type to put the result in. Will be bracket-indexed.
//          (Polyscope pretty much always uses glm::vec2/3 or std::array<>)
// class T: input array type (must be a 2D vector)
template <class O, class T>
O standardizeVector2D(const T& inputVec) {
  O out;
  out[0] = adaptorF_accessVector2Value<decltype(out[0]), 0, T>(inputVec);
  out[1] = adaptorF_accessVector2Value<decltype(out[0]), 1, T>(inputVec);
  return out;
}

// Convert a single fixed-size 2D vector
// class O: output vector type to put the result in. Will be bracket-indexed.
//          (Polyscope pretty much always uses glm::vec2/3 or std::array<>)
// class T: input array type (must be a 3D vector)
template <class O, class T>
O standardizeVector3D(const T& inputVec) {
  O out;
  out[0] = adaptorF_accessVector3Value<decltype(out[0]), 0, T>(inputVec);
  out[1] = adaptorF_accessVector3Value<decltype(out[0]), 1, T>(inputVec);
  out[2] = adaptorF_accessVector3Value<decltype(out[0]), 2, T>(inputVec);
  return out;
}

// Convert an array of scalar types
// class D: scalar data type
// class T: input array type
template <class D, class T>
std::vector<D> standardizeArray(const T& inputData) {
  std::vector<D> out;
  adaptorF_convertToStdVector<D, T>(inputData, out);
  return out;
}

// Convert an array of vector types
// class O: output inner vector type to put the result in. Will be bracket-indexed.
//          (Polyscope pretty much always uses glm::vec2/3, std::vector<>, or std::array<>)
// unsigned int D: dimension of inner vector type
// class T: input array type
template <class O, unsigned int D, class T>
std::vector<O> standardizeVectorArray(const T& inputData) {
  return adaptorF_convertArrayOfVectorToStdVector<O, D, T>(inputData);
}

// Convert a nested array where the inner types have variable length.
// class S: innermost scalar type for output
// class T: input nested array type
template <class S, class I, class T>
std::tuple<std::vector<S>, std::vector<I>> standardizeNestedList(const T& inputData) {
  return adaptorF_convertNestedArrayToStdVector<S, I>(inputData);
}

} // namespace polyscope

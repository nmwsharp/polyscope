// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/render/color_maps.h"
#include "polyscope/utilities.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

namespace polyscope {


// What is the meaningful range of these values?
// Used to set meaningful colormaps
// STANDARD: [-inf, inf], zero does not mean anything special (ie, position)
// SYMMETRIC: [-inf, inf], zero is special (ie, net profit/loss)
// MAGNITUDE: [0, inf], zero is special (ie, length of a vector)
enum class DataType { STANDARD = 0, SYMMETRIC, MAGNITUDE };

inline std::string defaultColorMap(DataType type);


// What is the meaningful scale of an R3 vector?
// Used to scale vector lengths in a meaningful way
// STANDARD: no special meaning
// AMBIENT: vector represent distances in the ambient space
enum class VectorType { STANDARD = 0, AMBIENT };


// Field magnitude type
template <class P>
struct FIELD_MAG {
  typedef double type;
};
template <>
struct FIELD_MAG<glm::vec3> {
  typedef float type;
};


template <typename T>
std::pair<typename FIELD_MAG<T>::type, typename FIELD_MAG<T>::type>
robustMinMax(const std::vector<T>& data, typename FIELD_MAG<T>::type rangeEPS = 1e-12);


// Map data in to the range [0,1]
template <typename T>
class AffineRemapper {

public:
  // Create a new remapper
  AffineRemapper(const std::vector<T>& data, DataType datatype = DataType::STANDARD);
  AffineRemapper(T offset, typename FIELD_MAG<T>::type scale);
  AffineRemapper(typename FIELD_MAG<T>::type minVal, typename FIELD_MAG<T>::type maxVal,
                 DataType datatype = DataType::STANDARD);
  AffineRemapper(); // identity mapper

  // Data that defines the map as f(x) = (x - offset) * scale
  T offset;
  typename FIELD_MAG<T>::type scale, minVal, maxVal;

  T map(const T& x);
  void setMinMax(const std::vector<T>& data); // useful when using identity mapper but want accurate bounds
  std::string printBounds();

  // Helpers for logic on templated fields
  static T one();
  static T zero();
};

} // namespace polyscope


#include "polyscope/affine_remapper.ipp"

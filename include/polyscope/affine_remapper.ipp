// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "glm/glm.hpp"

namespace polyscope {

inline std::string defaultColorMap(DataType type) {
  switch (type) {
  case DataType::STANDARD:
    return "viridis";
    break;
  case DataType::SYMMETRIC:
    return "coolwarm";
  case DataType::MAGNITUDE:
    return "blues";
    break;
  }
  return "viridis";
}

// Helpers used mainy to treat vectors as fields
namespace {


// Field "size" used to compare elements for SIGNED bigness
template <typename T>
typename FIELD_MAG<T>::type FIELD_BIGNESS(T x) {
  return x;
}
template <>
typename FIELD_MAG<glm::vec3>::type FIELD_BIGNESS(glm::vec3 x) {
  return glm::length(x);
}

// Multiplicative identity
template <typename T>
T FIELD_ONE() {
  return 1;
}
template <>
glm::vec3 FIELD_ONE() {
  return glm::vec3{1., 1., 1.};
}

// Additive identity
template <typename T>
T FIELD_ZERO() {
  return 0;
}
template <>
glm::vec3 FIELD_ZERO() {
  return glm::vec3{0., 0., 0.};
}
}; // namespace


template <typename T>
std::pair<typename FIELD_MAG<T>::type, typename FIELD_MAG<T>::type> robustMinMax(const std::vector<T>& data,
                                                                                 typename FIELD_MAG<T>::type rangeEPS) {

  if (data.size() == 0) {
    return std::make_pair(-1.0, 1.0);
  }

  // Compute max and min of data for mapping
  typename FIELD_MAG<T>::type minVal = std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  typename FIELD_MAG<T>::type maxVal = -std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  bool anyFinite = false;
  for (const T& x : data) {
    if (std::isfinite(FIELD_BIGNESS(x))) {
      minVal = std::min(minVal, FIELD_BIGNESS(x));
      maxVal = std::max(maxVal, FIELD_BIGNESS(x));
      anyFinite = true;
    }
  }
  if (!anyFinite) {
    return std::make_pair(-1.0, 1.0);
  }
  typename FIELD_MAG<T>::type maxMag = std::max(std::abs(minVal), std::abs(maxVal));

  // Hack to do less ugly things when constants (or near-constant) are passed in
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    typename FIELD_MAG<T>::type mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }

  return std::make_pair(minVal, maxVal);
}

template <typename T>
AffineRemapper<T>::AffineRemapper(T offset_, typename FIELD_MAG<T>::type scale_)
    : offset(offset_), scale(scale){

                       };

template <typename T>
AffineRemapper<T>::AffineRemapper()
    : offset(FIELD_ZERO<T>()), scale(1.0), minVal(std::numeric_limits<typename FIELD_MAG<T>::type>::quiet_NaN()),
      maxVal(std::numeric_limits<typename FIELD_MAG<T>::type>::quiet_NaN()){

      };

template <typename T>
AffineRemapper<T>::AffineRemapper(const std::vector<T>& data, DataType datatype) {
  // Compute max and min of data for mapping
  minVal = std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  T minElem;
  maxVal = -std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  for (const T& x : data) {
    if (FIELD_BIGNESS(x) < minVal) {
      minElem = x;
      minVal = FIELD_BIGNESS(x);
    }
    maxVal = std::max(maxVal, FIELD_BIGNESS(x));
  }
  typename FIELD_MAG<T>::type maxMag = std::max(std::abs(minVal), std::abs(maxVal));

  // Hack to do less ugly things when constants (or near-constant) are passed in
  typename FIELD_MAG<T>::type rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
    minElem = minVal * FIELD_ONE<T>();
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    typename FIELD_MAG<T>::type mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
    minElem = minVal * FIELD_ONE<T>();
  }
  maxMag = std::max(std::abs(minVal), std::abs(maxVal));

  if (datatype == DataType::STANDARD) {
    offset = minElem;
    scale = 1.0 / (maxVal - minVal);
  } else if (datatype == DataType::SYMMETRIC) {
    offset = -FIELD_ONE<T>() * maxMag;
    scale = 1.0 / (2 * maxMag);
  } else if (datatype == DataType::MAGNITUDE) {
    offset = FIELD_ZERO<T>();
    scale = 1.0 / (maxMag);
  }
}

template <typename T>
AffineRemapper<T>::AffineRemapper(typename FIELD_MAG<T>::type minVal, typename FIELD_MAG<T>::type maxVal,
                                  DataType datatype) {
  if (datatype == DataType::STANDARD) {
    offset = minVal * FIELD_ONE<T>();
    scale = 1.0 / (maxVal - minVal);
  } else if (datatype == DataType::SYMMETRIC) {
    typename FIELD_MAG<T>::type maxMag = std::max(std::abs(minVal), std::abs(maxVal));
    offset = -FIELD_ONE<T>() * maxMag;
    scale = 1.0 / (2 * maxMag);
  } else if (datatype == DataType::MAGNITUDE) {
    typename FIELD_MAG<T>::type maxMag = std::max(std::abs(minVal), std::abs(maxVal));
    offset = FIELD_ZERO<T>();
    scale = 1.0 / (maxMag);
  }
}

template <typename T>
void AffineRemapper<T>::setMinMax(const std::vector<T>& data) {
  minVal = std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  maxVal = -std::numeric_limits<typename FIELD_MAG<T>::type>::infinity();
  for (const T& x : data) {
    minVal = std::min(minVal, FIELD_BIGNESS(x));
    maxVal = std::max(maxVal, FIELD_BIGNESS(x));
  }
}

template <typename T>
inline T AffineRemapper<T>::map(const T& x) {
  return (x - offset) * scale;
}

template <typename T>
std::string AffineRemapper<T>::printBounds() {
  const size_t bSize = 32;
  std::array<char, bSize> b;
  snprintf(b.data(), bSize, "[%6.2e, %6.2e]", minVal, maxVal);
  return std::string(b.data());
}

}; // namespace polyscope

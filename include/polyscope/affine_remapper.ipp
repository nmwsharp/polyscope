#pragma once

#include "geometrycentral/vector3.h"

namespace polyscope {

// Helpers used mainy to treat vectors as fields
namespace {

// Field "size" used to compare elements for SIGNED bigness
template <typename T>
double FIELD_BIGNESS(T x) {
  return x;
}
template <>
double FIELD_BIGNESS(geometrycentral::Vector3 x) {
  return geometrycentral::norm(x);
}

// Multiplicative identity
template <typename T>
T FIELD_ONE() {
  return 1;
}
template <>
geometrycentral::Vector3 FIELD_ONE() {
  return geometrycentral::Vector3{1., 1., 1.};
}

// Additive identity
template <typename T>
T FIELD_ZERO() {
  return 0;
}
template <>
geometrycentral::Vector3 FIELD_ZERO() {
  return geometrycentral::Vector3::zero();
}
}; // namespace


template <typename T>
std::pair<double, double> robustMinMax(const std::vector<T>& data, double rangeEPS) {

  if (data.size() == 0) {
    return std::make_pair(-1.0, 1.0);
  }

  // Compute max and min of data for mapping
  double minVal = std::numeric_limits<double>::infinity();
  double maxVal = -std::numeric_limits<double>::infinity();
  bool anyFinite = false;
  for (const T& x : data) {
    if (std::isfinite(FIELD_BIGNESS(x))) {
      minVal = std::min(minVal, FIELD_BIGNESS(x));
      maxVal = std::max(maxVal, FIELD_BIGNESS(x));
      anyFinite = true;
    }
  }
  if(!anyFinite) {
    return std::make_pair(-1.0, 1.0);
  }
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));

  // Hack to do less ugly things when constants (or near-constant) are passed in
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
    maxVal = mid + maxMag * rangeEPS;
    minVal = mid - maxMag * rangeEPS;
  }

  return std::make_pair(minVal, maxVal);
}

template <typename T>
AffineRemapper<T>::AffineRemapper(T offset_, double scale_)
    : offset(offset_), scale(scale){

                       };

template <typename T>
AffineRemapper<T>::AffineRemapper()
    : offset(FIELD_ZERO<T>()), scale(1.0), minVal(std::numeric_limits<double>::quiet_NaN()),
      maxVal(std::numeric_limits<double>::quiet_NaN()){

      };

template <typename T>
AffineRemapper<T>::AffineRemapper(const std::vector<T>& data, DataType datatype) {
  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  T minElem;
  maxVal = -std::numeric_limits<double>::infinity();
  for (const T& x : data) {
    if (FIELD_BIGNESS(x) < minVal) {
      minElem = x;
      minVal = FIELD_BIGNESS(x);
    }
    maxVal = std::max(maxVal, FIELD_BIGNESS(x));
  }
  double maxMag = std::max(std::abs(minVal), std::abs(maxVal));

  // Hack to do less ugly things when constants (or near-constant) are passed in
  double rangeEPS = 1E-12;
  if (maxMag < rangeEPS) {
    maxVal = rangeEPS;
    minVal = -rangeEPS;
    minElem = minVal * FIELD_ONE<T>();
  } else if ((maxVal - minVal) / maxMag < rangeEPS) {
    double mid = (minVal + maxVal) / 2.0;
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
AffineRemapper<T>::AffineRemapper(double minVal, double maxVal, DataType datatype) {
  if (datatype == DataType::STANDARD) {
    offset = minVal * FIELD_ONE<T>();
    scale = 1.0 / (maxVal - minVal);
  } else if (datatype == DataType::SYMMETRIC) {
    double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
    offset = -FIELD_ONE<T>() * maxMag;
    scale = 1.0 / (2 * maxMag);
  } else if (datatype == DataType::MAGNITUDE) {
    double maxMag = std::max(std::abs(minVal), std::abs(maxVal));
    offset = FIELD_ZERO<T>();
    scale = 1.0 / (maxMag);
  }
}

template <typename T>
void AffineRemapper<T>::setMinMax(const std::vector<T>& data) {
  minVal = std::numeric_limits<double>::infinity();
  maxVal = -std::numeric_limits<double>::infinity();
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
  size_t bSize = 50;
  char b[bSize];
  snprintf(b, bSize, "[%6.2e, %6.2e]", minVal, maxVal);
  return std::string(b);
}

}; // namespace polyscope

#pragma once


namespace polyscope {

// Helpers used mainy to treat vectors as fields
namespace {
template <typename T>
T FIELD_ONE() {
  return 1;
}

template <typename T>
T FIELD_ZERO() {
  return 0;
}
};  // namespace

template <typename T>
AffineRemapper<T>::AffineRemapper(T offset_, double scale_)
    : offset(offset_),
      scale(scale){

      };

template <typename T>
AffineRemapper<T>::AffineRemapper()
    : offset(FIELD_ZERO<T>()),
      scale(1.0){

      };

template <typename T>
AffineRemapper<T>::AffineRemapper(const std::vector<T>& data,
                                  DataType datatype) {
  // Compute max and min of data for mapping
  minVal = std::numeric_limits<double>::infinity();
  T minElem;
  maxVal = -std::numeric_limits<double>::infinity();
  for (const T& x : data) {
    if (x < minVal) {
      minElem = x;
      minVal = x;
    }
    maxVal = std::max(maxVal, x);
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
AffineRemapper<T>::AffineRemapper(double minVal, double maxVal,
                                  DataType datatype) {
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

};  // namespace polyscope

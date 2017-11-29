#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace polyscope {


// What is the meaningful range of these values?
// Used to set meaningful colormaps
// STANDARD: [-inf, inf], zero does not mean anything special (ie, position)
// SYMMETRIC: [-inf, inf], zero is special (ie, net profit/loss)
// MAGNITUDE: [0, inf], zero is special (ie, length of a vector)
enum class DataType { STANDARD = 0, SYMMETRIC, MAGNITUDE};


// Map data in to the range [0,1]
template <typename T>
class AffineRemapper {

public:

    // Create a new remapper
    AffineRemapper(const std::vector<T>& data, DataType datatype=DataType::STANDARD);
    AffineRemapper(T offset, double scale);
    AffineRemapper(double minVal, double maxVal, DataType datatype=DataType::STANDARD);
    AffineRemapper();

    // Data that defines the map as f(x) = (x - offset) * scale
    T offset;
    double scale, minVal, maxVal;

    T map(const T& x);
    std::string printBounds();

    // Helpers for logic on templated fields
    static T one();
    static T zero();
};

}  // namespace polyscope


#include "polyscope/affine_remapper.ipp"
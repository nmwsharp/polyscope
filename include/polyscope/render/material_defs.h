// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/render/materials.h"

#include <array>

namespace polyscope {

namespace render {

// The arrays that hold the actual data. Stored as constants in render/bindata

extern const std::array<unsigned char, 54634> bindata_clay_r;
extern const std::array<unsigned char, 55016> bindata_clay_g;
extern const std::array<unsigned char, 55586> bindata_clay_b;
extern const std::array<unsigned char, 100903> bindata_clay_k;

extern const std::array<unsigned char, 65962> bindata_wax_r;
extern const std::array<unsigned char, 67015> bindata_wax_g;
extern const std::array<unsigned char, 68106> bindata_wax_b;
extern const std::array<unsigned char, 102435> bindata_wax_k;

extern const std::array<unsigned char, 71709> bindata_candy_r;
extern const std::array<unsigned char, 73863> bindata_candy_g;
extern const std::array<unsigned char, 75477> bindata_candy_b;
extern const std::array<unsigned char, 103065> bindata_candy_k;

extern const std::array<unsigned char, 489> bindata_flat_r;
extern const std::array<unsigned char, 489> bindata_flat_g;
extern const std::array<unsigned char, 489> bindata_flat_b;
extern const std::array<unsigned char, 489> bindata_flat_k;

} // namespace render
} // namespace polyscope

// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/materials.h"

#include <array>

namespace polyscope {

namespace render {

// The arrays that hold the actual data. Stored as constants in render/bindata
extern const std::array<unsigned char, 84934> bindata_clay_r;
extern const std::array<unsigned char, 84889> bindata_clay_g;
extern const std::array<unsigned char, 83655> bindata_clay_b;
extern const std::array<unsigned char, 161312> bindata_clay_k;

extern const std::array<unsigned char, 101212> bindata_wax_r;
extern const std::array<unsigned char, 101157> bindata_wax_g;
extern const std::array<unsigned char, 100294> bindata_wax_b;
extern const std::array<unsigned char, 164233> bindata_wax_k;

extern const std::array<unsigned char, 95767> bindata_candy_r;
extern const std::array<unsigned char, 96019> bindata_candy_g;
extern const std::array<unsigned char, 95002> bindata_candy_b;
extern const std::array<unsigned char, 161136> bindata_candy_k;

extern const std::array<unsigned char, 489> bindata_flat_r;
extern const std::array<unsigned char, 489> bindata_flat_g;
extern const std::array<unsigned char, 489> bindata_flat_b;
extern const std::array<unsigned char, 489> bindata_flat_k;

extern const std::array<unsigned char, 18071> bindata_mud;
extern const std::array<unsigned char, 20804> bindata_ceramic;
extern const std::array<unsigned char, 17500> bindata_jade;
extern const std::array<unsigned char, 29444> bindata_normal;

extern const std::array<unsigned char, 50022> bindata_concrete;


} // namespace render
} // namespace polyscope

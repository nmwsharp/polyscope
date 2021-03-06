#pragma once

#include <string>
#include <vector>
#include <array>

void parseVolumeDotMesh(std::string filename, std::vector<std::array<double,3>>& vertsOut, std::vector<std::array<int64_t,8>>& cellsOut);

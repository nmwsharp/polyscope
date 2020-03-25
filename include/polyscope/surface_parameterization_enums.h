// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

enum class ParamCoordsType { UNIT = 0, WORLD };                         // UNIT -> [0,1], WORLD -> length-valued
enum class ParamVizStyle { CHECKER = 0, GRID, LOCAL_CHECK, LOCAL_RAD }; // TODO add "UV" with test UV map

} // namespace polyscope

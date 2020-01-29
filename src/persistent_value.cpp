#include "polyscope/persistent_value.h"

namespace polyscope {
namespace detail {
// storage for persistent value global caches
// clang-format off
PersistentCache<double> persistentCache_double;
PersistentCache<float> persistentCache_float;
PersistentCache<bool> persistentCache_bool;
PersistentCache<glm::vec3> persistentCache_glmvec3;
PersistentCache<ScaledValue<double>> persistentCache_scaleddouble;
PersistentCache<ScaledValue<float>> persistentCache_scaledfloat;
PersistentCache<gl::ColorMapID> persistentCache_colormapID;
PersistentCache<ParamVizStyle> persistentCache_paramVizStyle;
// clang-format on
} // namespace detail
} // namespace polyscope

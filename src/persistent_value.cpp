#include "polyscope/persistent_value.h"

namespace polyscope {
namespace detail {
// storage for persistent value global caches
// clang-format off
PersistentCache<double> persistentCache_double;
PersistentCache<float> persistentCache_float;
PersistentCache<glm::vec3> persistentCache_glmvec3;
PersistentCache<ScaledValue<double>> persistentCache_scaleddouble;
PersistentCache<ScaledValue<float>> persistentCache_scaledfloat;
// clang-format on
} // namespace detail
} // namespace polyscope

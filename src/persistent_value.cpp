#include "polyscope/persistent_value.h"

#include "polyscope/render/color_maps.h"

namespace polyscope {
namespace detail {
// storage for persistent value global caches
// clang-format off
PersistentCache<double> persistentCache_double;
PersistentCache<float> persistentCache_float;
PersistentCache<bool> persistentCache_bool;
PersistentCache<std::string> persistentCache_string;
PersistentCache<glm::vec3> persistentCache_glmvec3;
PersistentCache<glm::mat4> persistentCache_glmmat4;
PersistentCache<ScaledValue<double>> persistentCache_scaleddouble;
PersistentCache<ScaledValue<float>> persistentCache_scaledfloat;
PersistentCache<ParamVizStyle> persistentCache_paramVizStyle;
PersistentCache<BackfacePolicy> persistentCache_backfacePolicy;
// clang-format on
} // namespace detail
} // namespace polyscope

#include "polyscope/persistent_value.h"

#include "polyscope/render/color_maps.h"

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
PersistentCache<Material> persistentCache_material;
PersistentCache<ParamVizStyle> persistentCache_paramVizStyle;
PersistentCache<const render::ValueColorMap*> persistentCache_colormap;
// clang-format on
} // namespace detail
} // namespace polyscope

#pragma once

#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

std::vector<ShaderStageSpecification>
applyShaderReplacements(const std::vector<ShaderStageSpecification>& stages,
                        const std::vector<ShaderReplacementRule>& replacementRules);

}
} // namespace polyscope

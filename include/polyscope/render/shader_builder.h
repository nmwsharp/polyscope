// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/engine.h"

namespace polyscope {
namespace render {

std::vector<ShaderStageSpecification>
applyShaderReplacements(const std::vector<ShaderStageSpecification>& stages,
                        const std::vector<ShaderReplacementRule>& replacementRules);

}
} // namespace polyscope

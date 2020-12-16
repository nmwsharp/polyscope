#include "polyscope/render/shader_builder.h"


namespace polyscope {
namespace render {

std::vector<ShaderStageSpecification>
applyShaderReplacements(const std::vector<ShaderStageSpecification>& stages,
                        const std::vector<ShaderReplacementRule>& replacementRules) {

  bool debugPrint = false;

  // accumulate the text to be inserted at each tag from all of the rules
  std::map<std::string, std::string> replacements;
  for (const ShaderReplacementRule& rule : replacementRules) {
    if (debugPrint) std::cout << "Replacement rule: " << rule.ruleName << std::endl;

    for (const std::pair<std::string, std::string>& r : rule.replacements) {
      std::string key = r.first;
      std::string value = r.second;

      if (replacements.find(key) == replacements.end()) {
        replacements[key] = "";
      }
      replacements[key] = replacements[key] + "// from rule: " + rule.ruleName + "\n" + value + "\n";
    }
  }

  const auto npos = std::string::npos;
  const std::string startTagToken = "${ ";
  const std::string endTagToken = " }$";

  // == Apply the replacements to the shader source
  std::vector<ShaderStageSpecification> replacedStages;
  for (ShaderStageSpecification stage : stages) { // iterate by value to modify
    std::string progText = stage.src;
    std::string resultText = "";

    while (!progText.empty()) {

      if (debugPrint) std::cout << "searching " << progText << std::endl;

      // Find the next tag in the program
      auto tagStart = progText.find(startTagToken);
      auto tagEnd = progText.find(endTagToken);

      if (tagStart != npos && tagEnd == npos) throw std::runtime_error("ShaderBuilder: no end tag matching start tag");
      if (tagStart == npos && tagEnd != npos) throw std::runtime_error("ShaderBuilder: no start tag matching end tag");

      // no more tags, concatenate in the rest of the source finish looping
      if (tagStart == npos && tagEnd == npos) {
        resultText += progText;
        progText = "";
      } else {

        if (debugPrint) std::cout << "FOUND TAG: " << tagStart << " " << tagEnd << std::endl;

        std::string srcBefore = progText.substr(0, tagStart);
        std::string tag = progText.substr(tagStart + startTagToken.size(), tagEnd - (tagStart + startTagToken.size()));
        std::string srcAfter = progText.substr(tagEnd + endTagToken.size(), npos);

        if (debugPrint) std::cout << "  TAG NAME: [" << tag << "]\n";

        resultText += srcBefore + "\n// tag ${ " + tag + " }$\n";
        if (replacements.find(tag) != replacements.end()) {
          resultText += replacements[tag];
          // std::cout << "  ADDING REPLACEMENT: [" << replacements[tag] << "]\n";
        }
        // resultText += "// END ADDIITIONS FROM TAG ${ " + tag + " $}\n";
        progText = srcAfter; // continue processing the remaining program text
      }
    }

    // For now, we put the uniform listings on the all stages, attributes on vertex shaders, and textures on fragment
    // shaders, since this is where they are mostly commonly used. These listings are only used internally by Polyscope
    // to check inputs, so this should be fine even if they happen to be used elsewhere.

    // == Union the uniforms
    std::vector<ShaderSpecUniform> replacedUniforms = stage.uniforms;
    for (const ShaderReplacementRule& rule : replacementRules) {
      for (ShaderSpecUniform newU : rule.uniforms) {

        // Look for a matching-named existing uniform
        bool existingFound = false;
        for (ShaderSpecUniform existingU : replacedUniforms) {
          if (existingU.name == newU.name) {
            // check for conflics
            if (existingU.type != newU.type) {
              throw std::runtime_error("ShaderBuilder: rule uniform [" + newU.name +
                                       "] conflicts with existing uniform of different type");
            }
            existingFound = true;
            break; // no need to continue, it already exists
          }
        }

        // No matching uniform found, append
        if (!existingFound) {
          replacedUniforms.push_back(newU);
        }
      }
    }

    // == Union the attributes
    std::vector<ShaderSpecAttribute> replacedAttributes = stage.attributes;
    if (stage.stage == ShaderStageType::Vertex) {
      for (const ShaderReplacementRule& rule : replacementRules) {
        for (ShaderSpecAttribute newA : rule.attributes) {

          // Look for a matching-named existing attribute
          bool existingFound = false;
          for (ShaderSpecAttribute existingA : replacedAttributes) {
            if (existingA.name == newA.name) {
              // check for conflics
              if (existingA.type != newA.type) {
                throw std::runtime_error("ShaderBuilder: rule attribute [" + newA.name +
                                         "] conflicts with existing attribute of different type");
              }
              if (existingA.arrayCount != newA.arrayCount) {
                throw std::runtime_error("ShaderBuilder: rule attribute [" + newA.name +
                                         "] conflicts with existing attribute of different array count");
              }
              existingFound = true;
              break; // no need to continue, it already exists
            }
          }

          // No matching attribute found, append
          if (!existingFound) {
            replacedAttributes.push_back(newA);
          }
        }
      }
    }


    // == Union the textures
    std::vector<ShaderSpecTexture> replacedTextures = stage.textures;
    if (stage.stage == ShaderStageType::Fragment) {
      for (const ShaderReplacementRule& rule : replacementRules) {
        for (ShaderSpecTexture newT : rule.textures) {

          // Look for a matching-named existing texture
          bool existingFound = false;
          for (ShaderSpecTexture existingT : replacedTextures) {
            if (existingT.name == newT.name) {
              // check for conflics
              if (existingT.dim != newT.dim) {
                throw std::runtime_error("ShaderBuilder: rule texture [" + newT.name +
                                         "] conflicts with existing texture of different dim");
              }
              existingFound = true;
              break; // no need to continue, it already exists
            }
          }

          // No matching texture found, append
          if (!existingFound) {
            replacedTextures.push_back(newT);
          }
        }
      }
    }


    // create a new specification, which is identical except for the replaced source text
    ShaderStageSpecification newStage{stage.stage, replacedUniforms, replacedAttributes, replacedTextures, resultText};
    replacedStages.push_back(newStage);
  }

  return replacedStages;
}

} // namespace render
} // namespace polyscope

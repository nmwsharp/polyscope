// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

namespace polyscope {

namespace state {

bool initialized = false;
std::string backend = "";
float lengthScale = 1.0;
std::tuple<glm::vec3, glm::vec3> boundingBox =
    std::tuple<glm::vec3, glm::vec3>{glm::vec3{-1., -1., -1.}, glm::vec3{1., 1., 1.}};
std::map<std::string, std::map<std::string, std::unique_ptr<Structure>>> structures;
std::map<std::string, std::unique_ptr<Group>> groups;
std::function<void()> userCallback = nullptr;
bool doDefaultMouseInteraction = true;

// Lists of things
std::vector<WeakHandle<Widget>> widgets;
std::vector<std::unique_ptr<SlicePlane>> slicePlanes;

} // namespace state
} // namespace polyscope

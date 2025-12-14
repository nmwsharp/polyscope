// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

namespace polyscope {

namespace state {


Context globalContext;

// Map all of the named global variables as references to the context struct
bool& initialized = globalContext.initialized;
std::string& backend = globalContext.backend;
std::map<std::string, std::map<std::string, std::unique_ptr<Structure>>>& structures = globalContext.structures;
std::map<std::string, std::unique_ptr<Group>>& groups = globalContext.groups;
float& lengthScale = globalContext.lengthScale;
std::tuple<glm::vec3, glm::vec3>& boundingBox = globalContext.boundingBox;
std::vector<std::unique_ptr<SlicePlane>>& slicePlanes = globalContext.slicePlanes;
std::vector<WeakHandle<Widget>>& widgets = globalContext.widgets;
std::function<void()>& userCallback = globalContext.userCallback;
std::function<void(const std::vector<std::string>&)> filesDroppedCallback = globalContext.filesDroppedCallback;


// deprecated, reference the new name in options
bool& doDefaultMouseInteraction = options::doDefaultMouseInteraction;

} // namespace state
} // namespace polyscope

// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "imgui.h"
#include "polyscope/polyscope.h"
#include "polyscope/group.h"

namespace polyscope {

Group::Group(std::string name_)
    : name(name_) {
  childrenGroups = std::vector<Group*>();
  childrenStructures = std::vector<Structure*>();
  parentGroup = nullptr;
}

Group::~Group(){
  // unparent all children
  for (Group* child : childrenGroups) {
    child->parentGroup = nullptr;
  }
  // remove onselves from parent
  if (parentGroup != nullptr) {
    parentGroup->removeChildGroup(this);
  }
};

void Group::buildUI() {

  if (ImGui::TreeNode(niceName().c_str())) {

    // Enabled checkbox
    bool enabledLocal = isEnabled();
    ImGui::Checkbox("Enabled", &enabledLocal);
    setEnabled(enabledLocal);

    // Call children buildUI
    for (Group* child : childrenGroups) {
      child->buildUI();
    }

    for (Structure* child : childrenStructures) {
      child->buildUI();
    }
    
    ImGui::TreePop();
  }
}

void Group::unparent() {
  if (parentGroup != nullptr) {
    parentGroup->removeChildGroup(this);
  }
  parentGroup = nullptr; // redundant, but explicit
}

void Group::removeChildGroup(Group* child) {
  auto it = std::find(childrenGroups.begin(), childrenGroups.end(), child);
  if (it != childrenGroups.end()) {
    (*it)->parentGroup = nullptr; // mark child as not having a parent anymore
    childrenGroups.erase(it);
  }
}

void Group::addChildGroup(Group* newChild) {
  // TODO: check cycles
  // TODO: if child is already in a group, remove it from that group
  newChild->parentGroup = this;
  childrenGroups.push_back(newChild);
}

void Group::addChildStructure(Structure* newChild) {
  childrenStructures.push_back(newChild);
}

int Group::isEnabled() {
  // TODO
  return 2;
}

Group* Group::setEnabled(bool newEnabled) {
  // TODO
  return this;
}

bool Group::isRootGroup() {
  return parentGroup == nullptr;
}

std::string Group::niceName() {
  return name;
}

} // namespace polyscope

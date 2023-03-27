// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/group.h"
#include "imgui.h"
#include "polyscope/polyscope.h"

#include "imgui_internal.h"

namespace {
bool CheckboxTristate(const char* label, int* v_tristate) {
  bool ret;
  if (*v_tristate == -1) {
    ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
    bool b = false;
    ret = ImGui::Checkbox(label, &b);
    if (ret) *v_tristate = 1;
    ImGui::PopItemFlag();
  } else {
    bool b = (*v_tristate != 0);
    ret = ImGui::Checkbox(label, &b);
    if (ret) *v_tristate = (int)b;
  }
  return ret;
}
}; // namespace


namespace polyscope {

Group::Group(std::string name_) : name(name_) {
  childrenGroups = std::vector<Group*>();
  childrenStructures = std::vector<Structure*>();
  parentGroup = nullptr;
}

Group::~Group() {
  // unparent all children
  for (Group* child : childrenGroups) {
    child->parentGroup = nullptr;
  }
  // clear vectors
  childrenGroups.clear();
  childrenStructures.clear();
  // remove oneself from parent
  if (parentGroup != nullptr) {
    parentGroup->removeChildGroup(this);
  }
};

void Group::buildUI() {

  // Set this treenode to open if there's children
  if (childrenGroups.size() > 0 || childrenStructures.size() > 0) {
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
  }

  if (ImGui::TreeNode(niceName().c_str())) {

    // Enabled checkbox
    int enabledLocal = isEnabled();
    if (enabledLocal == -2) {
      // no children, add greyed out text
      ImGui::TextDisabled("no child structures");
    } else {
      if (CheckboxTristate("Enabled", &enabledLocal)) {
        setEnabled(enabledLocal);
      }
    }

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

void Group::addChildGroup(Group* newChild) {
  // TODO: Daniel - check for cycles
  if (getTopLevelGrandparent() == newChild) {
    exception("Attempted to make group " + newChild->name + " a child of " + name +
              ", but this would create a cycle (group " + name + " is already a descendant of " + newChild->name + ")");
    return;
  }
  // if child is already in a group, remove it from that group
  newChild->unparent();
  newChild->parentGroup = this;
  childrenGroups.push_back(newChild);
}

void Group::addChildStructure(Structure* newChild) { childrenStructures.push_back(newChild); }

void Group::removeChildGroup(Group* child) {
  auto it = std::find(childrenGroups.begin(), childrenGroups.end(), child);
  if (it != childrenGroups.end()) {
    (*it)->parentGroup = nullptr; // mark child as not having a parent anymore
    childrenGroups.erase(it);
  }
}

void Group::removeChildStructure(Structure* child) {
  auto it = std::find(childrenStructures.begin(), childrenStructures.end(), child);
  if (it != childrenStructures.end()) {
    childrenStructures.erase(it);
  }
}

Group* Group::getTopLevelGrandparent() {
  Group* current = this;
  while (current->parentGroup != nullptr) {
    current = current->parentGroup;
  }
  return current;
}

int Group::isEnabled() {
  // return values:
  // 0: all children disabled
  // 1: all children enabled
  // -1: some children enabled, some disabled
  // -2: no children
  // (these -2 groups should not have a checkbox in the UI - there's nothing to enable / disable -
  // unless we added a is_enabled state for empty groups, but this could lead to edge cases)
  bool any_children_enabled = false;
  bool any_children_disabled = false;
  // check all structure children
  for (Structure* child : childrenStructures) {
    if (child->isEnabled()) {
      any_children_enabled = true;
    } else {
      any_children_disabled = true;
    }
  }
  // check all group children
  for (Group* child : childrenGroups) {
    if (child->isEnabled() == 1) {
      any_children_enabled = true;
    } else if (child->isEnabled() == 0) {
      any_children_disabled = true;
    } else if (child->isEnabled() == -1) {
      any_children_enabled = true;
      any_children_disabled = true;
    } else if (child->isEnabled() == -2) {
    } else { // huh?
      exception("Unexpected return value from Group::isEnabled()");
    }
  }

  int result = 0;
  if (!any_children_enabled && any_children_disabled) {
    result = 0;
  }
  if (any_children_enabled && !any_children_disabled) {
    result = 1;
  }
  if (any_children_enabled && any_children_disabled) {
    result = -1;
  }
  if (!any_children_enabled && !any_children_disabled) {
    result = -2;
  }
  return result;
}

Group* Group::setEnabled(bool newEnabled) {
  // set all structure children to enabled
  for (Structure* child : childrenStructures) {
    child->setEnabled(newEnabled);
  }
  // set all group children to enabled
  for (Group* child : childrenGroups) {
    child->setEnabled(newEnabled);
  }
  return this;
}

bool Group::isRootGroup() { return parentGroup == nullptr; }

std::string Group::niceName() { return name; }

} // namespace polyscope

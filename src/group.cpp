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

Group::Group(std::string name_) : name(name_) {}

Group::~Group() {
  // unparent all children
  for (std::weak_ptr<Group>& childWeak : childrenGroups) {
    std::shared_ptr<Group> child = childWeak.lock();
    if (child) {
      child->parentGroup.reset();
    }
  }
  // remove oneself from parent
  if (!parentGroup.expired()) {
    parentGroup.lock()->removeChildGroup(this);
  }
};

void Group::buildUI() {
  cullExpiredChildren();

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
    for (std::weak_ptr<Group>& childWeak : childrenGroups) {
      std::shared_ptr<Group> child = childWeak.lock();
      if (child) {
        child->buildUI();
      }
    }

    for (std::weak_ptr<Structure>& childWeak : childrenStructures) {
      std::shared_ptr<Structure> child = childWeak.lock();
      if (child) {
        child->buildUI();
      }
    }

    ImGui::TreePop();
  }
}

void Group::unparent() {
  cullExpiredChildren();

  if (!parentGroup.expired()) {
    std::shared_ptr<Group> parent = parentGroup.lock();
    if (parent) {
      parent->buildUI();
    }

    parent->removeChildGroup(this);
  }

  parentGroup.reset(); // redundant, but explicit
}

void Group::addChildGroup(std::weak_ptr<Group> newChildWeak) {
  cullExpiredChildren();

  std::shared_ptr<Group> newChild = newChildWeak.lock();
  if (!newChild) return;

  // if child is already in a group, remove it from that group
  newChild->unparent();

  if (getTopLevelGrandparent() == newChild.get()) {
    exception("Attempted to make group " + newChild->name + " a child of " + name +
              ", but this would create a cycle (group " + name + " is already a descendant of " + newChild->name + ")");
    return;
  }

  // assign to the new group
  newChild->parentGroup = state::groups[name]; // we want a weak pointer to the shared ptr
  childrenGroups.push_back(newChildWeak);
}

void Group::addChildStructure(std::weak_ptr<Structure> newChild) {
  cullExpiredChildren();
  childrenStructures.push_back(newChild);
}

void Group::removeChildGroup(Group* child) {
  cullExpiredChildren();

  childrenGroups.erase(std::remove_if(childrenGroups.begin(), childrenGroups.end(),
                                      [&](const std::weak_ptr<Group>& g_weak) {
                                        std::shared_ptr<Group> g = g_weak.lock();
                                        if (!g) return false;
                                        if (g.get() == child) {
                                          // mark child as not having a parent anymore
                                          g->parentGroup.reset();
                                          return true;
                                        }
                                        return false;
                                      }),
                       childrenGroups.end());
}

void Group::removeChildStructure(Structure* child) {
  cullExpiredChildren();

  childrenStructures.erase(std::remove_if(childrenStructures.begin(), childrenStructures.end(),
                                          [&](const std::weak_ptr<Structure>& s_weak) {
                                            std::shared_ptr<Structure> s = s_weak.lock();
                                            if (!s) return false;
                                            return (s.get() == child);
                                          }),
                           childrenStructures.end());
}

Group* Group::getTopLevelGrandparent() {
  cullExpiredChildren();
  Group* current = this;
  while (!current->parentGroup.expired()) {
    current = current->parentGroup.lock().get();
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

  cullExpiredChildren();

  bool any_children_enabled = false;
  bool any_children_disabled = false;
  // check all structure children

  for (std::weak_ptr<Structure>& childWeak : childrenStructures) {
    std::shared_ptr<Structure> child = childWeak.lock();
    if (child) {
      if (child->isEnabled()) {
        any_children_enabled = true;
      } else {
        any_children_disabled = true;
      }
    }
  }

  // check all group children
  for (std::weak_ptr<Group>& groupWeak : childrenGroups) {
    std::shared_ptr<Group> group = groupWeak.lock();
    if (group) {
      switch (group->isEnabled()) {
      case 1:
        any_children_enabled = true;
        break;
      case 0:
        any_children_disabled = true;
        break;
      case -1:
        any_children_enabled = true;
        any_children_disabled = true;
        break;
      case -2:
        break;
      default:
        exception("Unexpected return value from Group::isEnabled()");
      }
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

void Group::cullExpiredChildren() {

  if (parentGroup.expired()) {
    parentGroup.reset();
  }

  // remove any child groups which are expired
  // (erase-remove idiom)
  childrenGroups.erase(std::remove_if(childrenGroups.begin(), childrenGroups.end(),
                                      [](const std::weak_ptr<Group>& g) { return g.expired(); }),
                       childrenGroups.end());

  // remove any child structures which are expired
  // (erase-remove idiom)
  childrenStructures.erase(std::remove_if(childrenStructures.begin(), childrenStructures.end(),
                                          [](const std::weak_ptr<Structure>& s) { return s.expired(); }),
                           childrenStructures.end());
}

Group* Group::setEnabled(bool newEnabled) {
  for (std::weak_ptr<Group>& childWeak : childrenGroups) {
    std::shared_ptr<Group> child = childWeak.lock();
    if (child) {
      child->setEnabled(newEnabled);
    }
  }

  for (std::weak_ptr<Structure>& childWeak : childrenStructures) {
    std::shared_ptr<Structure> child = childWeak.lock();
    if (child) {
      child->setEnabled(newEnabled);
    }
  }
  return this;
}

bool Group::isRootGroup() { return !parentGroup.expired(); }

std::string Group::niceName() { return name; }

} // namespace polyscope

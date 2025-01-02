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

Group::Group(std::string name_)
    : name(name_), showChildDetails(uniqueName() + "showChildDetails", true),
      hideDescendantsFromStructureLists(uniqueName() + "hideDescendantsFromStructureLists", false) {}

Group::~Group() {
  // unparent all children
  for (WeakHandle<Group>& childWeak : childrenGroups) {
    if (childWeak.isValid()) {
      Group& child = childWeak.get();
      child.parentGroup.reset();
    }
  }
  // remove oneself from parent
  if (parentGroup.isValid()) {
    parentGroup.get().removeChildGroup(*this);
  }
};

void Group::buildUI() {
  cullExpiredChildren();

  // Set this treenode to open if there's children
  if (childrenGroups.size() > 0 || childrenStructures.size() > 0) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
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

      // Options popup
      ImGui::SameLine();
      if (ImGui::Button("Options")) {
        ImGui::OpenPopup("OptionsPopup");
      }
      if (ImGui::BeginPopup("OptionsPopup")) {

        if (ImGui::MenuItem("Show child details", NULL, getShowChildDetails())) {
          setShowChildDetails(!getShowChildDetails());
        }
        if (ImGui::MenuItem("Hide descendants from structure lists", NULL, getHideDescendantsFromStructureLists())) {
          setHideDescendantsFromStructureLists(!getHideDescendantsFromStructureLists());
        }
        ImGui::EndPopup();
      }
    }

    // Call children buildUI
    if (getShowChildDetails()) {
      for (WeakHandle<Group>& childWeak : childrenGroups) {
        if (childWeak.isValid()) {
          Group& child = childWeak.get();
          child.buildUI();
        }
      }

      for (WeakHandle<Structure>& childWeak : childrenStructures) {
        if (childWeak.isValid()) {
          Structure& child = childWeak.get();
          child.buildUI();
        }
      }
    }

    ImGui::TreePop();
  }
}

void Group::unparent() {
  cullExpiredChildren();

  if (parentGroup.isValid()) {
    Group& parent = parentGroup.get();
    parent.buildUI();
    parent.removeChildGroup(*this);
  }

  parentGroup.reset(); // redundant, but explicit
}

void Group::addChildGroup(Group& newChild) {
  cullExpiredChildren();

  // if child is already in a group, remove it from that group
  newChild.unparent();

  if (getTopLevelGrandparent() == &newChild) {
    exception("Attempted to make group " + newChild.name + " a child of " + name +
              ", but this would create a cycle (group " + name + " is already a descendant of " + newChild.name + ")");
    return;
  }

  // assign to the new group
  newChild.parentGroup = this->getWeakHandle<Group>(); // we want a weak pointer to the shared ptr
  childrenGroups.push_back(newChild.getWeakHandle<Group>());
}

void Group::addChildStructure(Structure& newChild) {
  cullExpiredChildren();
  childrenStructures.push_back(newChild.getWeakHandle<Structure>());
}

void Group::removeChildGroup(Group& child) {
  cullExpiredChildren();

  childrenGroups.erase(std::remove_if(childrenGroups.begin(), childrenGroups.end(),
                                      [&](const WeakHandle<Group>& gWeak) {
                                        if (!gWeak.isValid()) return false;
                                        if (&gWeak.get() == &child) {
                                          // mark child as not having a parent anymore
                                          gWeak.get().parentGroup.reset();
                                          return true;
                                        }
                                        return false;
                                      }),
                       childrenGroups.end());
}

void Group::removeChildStructure(Structure& child) {
  cullExpiredChildren();

  childrenStructures.erase(std::remove_if(childrenStructures.begin(), childrenStructures.end(),
                                          [&](const WeakHandle<Structure>& sWeak) {
                                            if (!sWeak.isValid()) return false;
                                            return (&sWeak.get() == &child);
                                          }),
                           childrenStructures.end());
}

Group* Group::getTopLevelGrandparent() {
  cullExpiredChildren();
  Group* current = this;
  while (current->parentGroup.isValid()) {
    current = &current->parentGroup.get();
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

  for (WeakHandle<Structure>& childWeak : childrenStructures) {
    if (childWeak.isValid()) {
      Structure& child = childWeak.get();
      if (child.isEnabled()) {
        any_children_enabled = true;
      } else {
        any_children_disabled = true;
      }
    }
  }

  // check all group children
  for (WeakHandle<Group>& groupWeak : childrenGroups) {
    if (groupWeak.isValid()) {
      Group& group = groupWeak.get();
      switch (group.isEnabled()) {
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

  if (!parentGroup.isValid()) {
    parentGroup.reset();
  }

  // remove any child groups which are expired
  // (erase-remove idiom)
  childrenGroups.erase(std::remove_if(childrenGroups.begin(), childrenGroups.end(),
                                      [](const WeakHandle<Group>& g) { return !g.isValid(); }),
                       childrenGroups.end());

  // remove any child structures which are expired
  // (erase-remove idiom)
  childrenStructures.erase(std::remove_if(childrenStructures.begin(), childrenStructures.end(),
                                          [](const WeakHandle<Structure>& s) { return !s.isValid(); }),
                           childrenStructures.end());
}

void Group::appendStructuresToSkip(std::unordered_set<Structure*>& skipSet) {
  if (getHideDescendantsFromStructureLists()) {
    appendAllDescendants(skipSet);
  }
}

void Group::appendAllDescendants(std::unordered_set<Structure*>& skipSet) {
  for (WeakHandle<Group>& childWeak : childrenGroups) {
    if (childWeak.isValid()) {
      Group& child = childWeak.get();
      child.appendAllDescendants(skipSet);
    }
  }

  for (WeakHandle<Structure>& childWeak : childrenStructures) {
    if (childWeak.isValid()) {
      Structure& child = childWeak.get();
      skipSet.insert(&child);
    }
  }
}

Group* Group::setEnabled(bool newEnabled) {
  for (WeakHandle<Group>& childWeak : childrenGroups) {
    if (childWeak.isValid()) {
      Group& child = childWeak.get();
      child.setEnabled(newEnabled);
    }
  }

  for (WeakHandle<Structure>& childWeak : childrenStructures) {
    if (childWeak.isValid()) {
      Structure& child = childWeak.get();
      child.setEnabled(newEnabled);
    }
  }
  return this;
}

Group* Group::setShowChildDetails(bool newVal) {
  showChildDetails = newVal;
  return this;
}
bool Group::getShowChildDetails() { return showChildDetails.get(); }

Group* Group::setHideDescendantsFromStructureLists(bool newVal) {
  hideDescendantsFromStructureLists = newVal;
  return this;
}
bool Group::getHideDescendantsFromStructureLists() { return hideDescendantsFromStructureLists.get(); }

bool Group::isRootGroup() { return !parentGroup.isValid(); }

std::string Group::niceName() { return name; }

std::string Group::uniqueName() { return "Group#" + name + "#"; }

} // namespace polyscope

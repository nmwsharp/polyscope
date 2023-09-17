// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <memory>
#include <string>

#include "polyscope/structure.h"
#include "polyscope/weak_handle.h"

namespace polyscope {

// Groups track collections of structures (or other groups) which can be toggled together.
//
// Groups are non-owning. Any contained structures continue their normal lifetime unaffected
// by the group. A structure can be in 0, 1, or multiple groups, and removing it from a group
// does not destroy the structure.

class Group : public virtual WeakReferrable {

public:
  Group(std::string name);
  ~Group();

  // Draw the ImGUI ui elements
  void buildUI(); // draws the tree node and enabled checkbox, and calls
                  // buildUI() for all children.

  // Is the group being displayed (0 no, 1 some children, 2 all children)
  int isEnabled();
  Group* setEnabled(bool newEnabled);

  void addChildGroup(Group& newChild);
  void addChildStructure(Structure& newChild);
  void removeChildGroup(Group& child);
  void removeChildStructure(Structure& child);
  void unparent();

  bool isRootGroup();
  Group* getTopLevelGrandparent();

  std::string niceName();

  // === Member variables ===
  WeakHandle<Group> parentGroup; // the parent group of this group (if null, this is a root group)
  const std::string name;        // a name for this group, which must be unique amongst groups on `parent`
  std::vector<WeakHandle<Group>> childrenGroups;
  std::vector<WeakHandle<Structure>> childrenStructures;

protected:
  void cullExpiredChildren(); // remove any child
};


} // namespace polyscope

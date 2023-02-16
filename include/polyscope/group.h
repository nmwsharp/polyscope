// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/structure.h"

#include <string>

namespace polyscope {

// A 'quantity' (in Polyscope terminology) is data which is associated with a structure; any structure might have many
// quantities. For instance a mesh structure might have a scalar quantity associated with it, or a point cloud might
// have a vector field quantity associated with it.

class Group {
public:
  Group(std::string name);
  ~Group();

  // Draw the ImGUI ui elements
  void buildUI();       // draws the tree node and enabled checkbox, and calls
                                // buildUI() for all children.

  // Is the group being displayed (0 no, 1 some children, 2 all children)
  int isEnabled();
  Group* setEnabled(bool newEnabled);

  void addChildGroup(Group* newChild);
  void addChildStructure(Structure* newChild);
  void removeChildGroup(Group* child);
  void unparent();

  bool isRootGroup();

  std::string niceName();

  // === Member variables ===
  Group* parentGroup; // the parent group of this group (if null, this is a root group)
  const std::string name; // a name for this group, which must be unique amongst groups on `parent`
  std::vector<Group*> childrenGroups;
  std::vector<Structure*> childrenStructures;

};


} // namespace polyscope

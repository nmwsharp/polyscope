// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/weak_handle.h"

namespace polyscope {

// A base class for widgets in the scene. These need to be tracked globally so they drawn/handled in the user
// interaction loop. The constructor/destructors take care of inserting the widget in to a global registery.
class Widget : public virtual WeakReferrable {

public:
  Widget();
  virtual ~Widget();

  // No copy constructor/assignment
  Widget(const Widget&) = delete;
  Widget& operator=(const Widget&) = delete;

  virtual void draw();
  virtual bool interact(); // returns true if the mouse input was consumed
  virtual void buildGUI();

}; // namespace polyscope
} // namespace polyscope

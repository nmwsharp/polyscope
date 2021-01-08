#pragma once

namespace polyscope {

// A base class for widgets in the scene. These need to be tracked globally so they drawn/handled in the user
// interaction loop. The constructor/destructors take care of inserting the widget in to a global registery.
class Widget {

public:
  Widget();
  ~Widget();

  // No copy constructor/assignment
  Widget(const Widget&) = delete;
  Widget& operator=(const Widget&) = delete;

  virtual void draw();
  virtual bool interact(); // returns true if the mouse input was consumed
  virtual void buildGUI();

}; // namespace polyscope
} // namespace polyscope

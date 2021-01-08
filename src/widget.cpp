#include "polyscope/widget.h"

#include "polyscope/polyscope.h"

namespace polyscope {

Widget::Widget() { state::widgets.insert(this); }

Widget::~Widget() {
  auto pos = state::widgets.find(this);
  if (pos == state::widgets.end()) return;
  state::widgets.erase(pos);
}


void Widget::draw() {}
bool Widget::interact() { return false; }
void Widget::buildGUI() {}

} // namespace polyscope

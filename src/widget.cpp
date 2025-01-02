// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/widget.h"

#include "polyscope/polyscope.h"

namespace polyscope {

Widget::Widget() { state::widgets.push_back(getWeakHandle<Widget>(this)); }

Widget::~Widget() {}

void Widget::draw() {}
bool Widget::interact() { return false; }
void Widget::buildGUI() {}

} // namespace polyscope

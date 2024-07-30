#include "layout.h"

void gluten::layout::add_layout_child(element* child) { m_children.insert(child); }

void gluten::layout::remove_layout_child(element* child) { m_children.erase(child); }

#include "toolbar.h"

gluten::toolbar::toolbar(const layout_type& type) : layout(type, anchor_preset::stretch_full) {}

auto gluten::toolbar::render_element(const element_render_info& renderInfo) -> bool { return false; }
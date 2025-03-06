#pragma once

#include "gluten/widgets/widget.h"

class details_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(details_widget, "Details Widget")

public:
    virtual void render_implementation() override;
};

#pragma once

#include "pch.h"

class database_widget : public gluten::widget
{
    WIDGET_CONSTRUCT(database_widget, "Database Widget")

protected:
    auto render_implementation() -> void override;
};
#pragma once

#include "pch.h"

#include "TracyView.hpp"

class profiler_widget : public gluten::widget
{
    WIDGET_CONSTRUCT(profiler_widget)

protected:
    auto start_implementation() -> void override;
    auto render_implementation() -> void override;
    auto end_implementation() -> void override;

private:
    std::unique_ptr<tracy::View> m_tracy;
    tracy::Config m_tracyConfig;
};
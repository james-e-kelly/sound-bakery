#include "widget_subsystem.h"

#include "widgets/root_widget.h"
#include "imgui.h"

int widget_subsystem::init()
{
    m_rootWidget = add_widget_class<root_widget>();
    return 0;
}

void widget_subsystem::tick(double deltaTime)
{
    assert(m_widgets.size() > 0);

    for (int index = m_widgets.size() - 1; index >= 0; index--)
    {
        std::unique_ptr<widget>& widget = m_widgets[index];

        if (!widget->has_started())
        {
            widget->start();
        }

        widget->tick(deltaTime);

        // We destroy before rendering
        // Ticking gives the widget the chance to kill itself
        // And after that we don't want to render it
        if (widget->m_wantsDestroy)
        {
            widget->m_onDestroy.Broadcast(widget.get());
            m_widgets.erase(m_widgets.begin() + index);
            continue;
        }

        widget->render();
    }
}

void widget_subsystem::exit()
{
    for (std::unique_ptr<widget>& widget : m_widgets)
    {
        widget->end();
    }
}

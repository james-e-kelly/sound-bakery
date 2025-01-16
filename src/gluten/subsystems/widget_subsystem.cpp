#include "widget_subsystem.h"

#include "gluten/widgets/root_widget.h"
#include "imgui.h"

using namespace gluten;

void widget_subsystem::tick(double deltaTime)
{
    ZoneScoped;

    assert(m_widgets.size() > 0);

    for (int index = m_widgets.size() - 1; index >= 0; index--)
    {
        std::weak_ptr<gluten::widget>& widget = m_widgets[index];

        if (std::shared_ptr<gluten::widget> sharedWidget = widget.lock())
        {
            if (!sharedWidget->has_started())
            {
                sharedWidget->start();
            }

            sharedWidget->tick(deltaTime);

            // We destroy before rendering
            // Ticking gives the widget the chance to kill itself
            // And after that we don't want to render it
            if (sharedWidget->m_wantsDestroy)
            {
                sharedWidget->m_onDestroy.Broadcast(sharedWidget.get());
                m_widgets.erase(m_widgets.begin() + index);
                continue;
            }

            sharedWidget->render();
        }
        else
        {
            m_widgets.erase(m_widgets.begin() + index);
        }
    }
}

void widget_subsystem::exit()
{
    ZoneScoped;

    for (std::weak_ptr<widget>& widget : m_widgets)
    {
        if (std::shared_ptr<gluten::widget> sharedWidget = widget.lock())
        {
            sharedWidget->end();
        }
    }
}

void gluten::widget_subsystem::set_root_widget(widget* rootWidget) { m_rootWidget = rootWidget->shared_from_this(); }

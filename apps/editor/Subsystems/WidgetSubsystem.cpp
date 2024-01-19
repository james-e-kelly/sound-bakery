#include "WidgetSubsystem.h"

#include "Widgets/RootWidget.h"
#include "imgui.h"

int WidgetSubsystem::Init()
{
    m_rootWidget = AddWidgetClass<RootWidget>();
    return 0;
}

void WidgetSubsystem::Tick(double deltaTime)
{
    assert(m_widgets.size() > 0);

    for (int index = m_widgets.size() - 1; index >= 0; index--)
    {
        std::unique_ptr<Widget>& widget = m_widgets[index];

        if (!widget->HasStarted())
        {
            widget->Start();
        }

        widget->Tick(deltaTime);

        // We destroy before rendering
        // Ticking gives the widget the chance to kill itself
        // And after that we don't want to render it
        if (widget->m_wantsDestroy)
        {
            widget->m_OnDestroy.Broadcast(widget.get());
            m_widgets.erase(m_widgets.begin() + index);
            continue;
        }

        widget->Render();
    }
}

void WidgetSubsystem::Exit()
{
    for (std::unique_ptr<Widget>& widget : m_widgets)
    {
        widget->End();
    }
}

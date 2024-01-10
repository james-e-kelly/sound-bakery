#include "Widget.h"

#include "App/App.h"

#include "Subsystems/WidgetSubsystem.h"
#include "imgui.h"

Widget::Widget(WidgetSubsystem* parentSubsystem)
: m_parentSubsystem(parentSubsystem)
{

}

Widget::Widget(Widget* parentWidget)
: m_parentWidget(parentWidget)
{

}

void Widget::Start()
{
    m_hasStarted = true;
}

void Widget::RenderChildren()
{
    for (std::unique_ptr<Widget>& child : m_childWidgets)
    {
        child->Render();
    }
}

App* Widget::GetApp() const
{
    const Widget* currentWidget = this;
    
    while (currentWidget->GetParentSubsystem() == nullptr)
    {
        currentWidget = currentWidget->GetParentWidget();
    }
    
    return currentWidget->GetParentSubsystem()->GetApp();
}

Widget* Widget::GetParentWidget() const
{
    return m_parentWidget;
}

WidgetSubsystem* Widget::GetParentSubsystem() const
{
    return m_parentSubsystem;
}

void Widget::Destroy()
{
    m_wantsDestroy = true;
}

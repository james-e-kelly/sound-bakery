#include "widget.h"

#include "app/app.h"
#include "imgui.h"
#include "subsystems/widget_subsystem.h"

using namespace gluten;

widget::widget(widget_subsystem* parentSubsystem) : m_parentSubsystem(parentSubsystem) {}

widget::widget(widget* parentWidget) : m_parentWidget(parentWidget) {}

void widget::start() { m_hasStarted = true; }

void widget::render_children()
{
    for (std::unique_ptr<widget>& child : m_childWidgets)
    {
        child->render();
    }
}

gluten::app* widget::get_app() const
{
    const widget* currentWidget = this;

    while (currentWidget->get_parent_subsystem() == nullptr)
    {
        currentWidget = currentWidget->get_parent_widget();
    }

    return currentWidget->get_parent_subsystem()->get_app();
}

widget* widget::get_parent_widget() const { return m_parentWidget; }

widget_subsystem* widget::get_parent_subsystem() const { return m_parentSubsystem; }

void widget::destroy() { m_wantsDestroy = true; }

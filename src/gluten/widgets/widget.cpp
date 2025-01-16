#include "widget.h"

#include "app/app.h"
#include "imgui.h"
#include "subsystems/widget_subsystem.h"

using namespace gluten;

widget::widget(widget_subsystem* parentSubsystem) : m_parentSubsystem(parentSubsystem) {}

widget::widget(widget* parentWidget) : m_parentWidget(parentWidget) {}

auto widget::start() -> void
{ 
    ZoneScoped;

    if (!m_hasStarted)
    {
        start_implementation();
        m_hasStarted = true; 
    }
}

auto widget::tick(double deltaTime) -> void
{
    ZoneScoped;

    if (m_hasStarted)
    {
        tick_implementation(deltaTime);
        for (std::weak_ptr<widget>& child : m_childWidgets)
        {
            if (std::shared_ptr<widget> sharedChild = child.lock())
            {
                sharedChild->tick(deltaTime);
            }
        }
    }
}

auto widget::render() -> void
{
    ZoneScoped;

    if (m_hasStarted)
    {
        render_implementation();

        if (m_autoRenderChildren)
        {
            render_children();
        }
    }
}

void gluten::widget::end()
{
    ZoneScoped;

    if (m_hasStarted && !m_hasEnded)
    {
        end_implementation();
        for (std::weak_ptr<widget>& child : m_childWidgets)
        {
            if (std::shared_ptr<widget> sharedChild = child.lock())
            {
                sharedChild->end();
            }
        }
        m_hasEnded = true;
    }
}

void widget::render_children()
{
    ZoneScoped;

    for (std::weak_ptr<widget>& child : m_childWidgets)
    {
        if (std::shared_ptr<widget> sharedChild = child.lock())
        {
            sharedChild->render();
        }
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

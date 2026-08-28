#include "widget.h"

#include "app/app.h"
#include "imgui.h"
#include "subsystems/widget_subsystem.h"

using namespace gluten;

widget::widget(widget_subsystem* parentSubsystem, const std::string& name) : m_parentSubsystem(parentSubsystem), m_widgetName(name) {}

widget::widget(widget* parentWidget, const std::string& name) : m_parentWidget(parentWidget), m_widgetName(name) {}

auto widget::start() -> void
{
    ZoneScoped;

    if (!m_hasStarted)
    {
        start_implementation();
        m_hasStarted = true;

        refresh_style_implementation(); // Only refresh ourselves. Children will be refreshed by their start function

        for (auto& child : m_childWidgets)
        {
            if (std::shared_ptr<widget> sharedChild = child.second.lock())
            {
                sharedChild->start();
            }
        }
    }
}

auto widget::tick(double deltaTime) -> void
{
    ZoneScoped;

    if (m_hasStarted)
    {
        bool doTick = false;

        if (m_tickFrequency > 0.0)
        {
            m_timeSinceLastTick += deltaTime;

            if (m_timeSinceLastTick >= m_tickFrequency)
            {
                deltaTime           = m_timeSinceLastTick;
                m_timeSinceLastTick = 0.0;
                doTick              = true;
            }
        }
        else
        {
            doTick = true;
        }

        if (doTick)
        {
            tick_implementation(deltaTime);
            for (auto& child : m_childWidgets)
            {
                if (std::shared_ptr<widget> sharedChild = child.second.lock())
                {
                    sharedChild->tick(deltaTime);
                }
            }
        }
    }
}

auto widget::refresh_style() -> void
{
    refresh_style_implementation();

    for (auto& child : m_childWidgets)
    {
        if (std::shared_ptr<widget> sharedChild = child.second.lock())
        {
            sharedChild->refresh_style();
        }
    }
}

auto widget::render() -> void
{
    ZoneScoped;

    if (is_visible())
    {
        if (m_hasStarted)
        {
            render_implementation();

            if (m_autoRenderChildren)
            {
                render_children();
            }
        }
    }
}

auto widget::render_menu() -> void
{
    ZoneScoped;

    if (m_hasStarted)
    {
        if (s_renderMenu)
        {
            if (ImGui::BeginMenu(s_fileMenuName))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(s_editMenuName))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(s_windowsMenuName))
            {
                if (m_inToolbar)
                {
                    ImGui::MenuItem(m_widgetName.c_str(), nullptr, &m_visible);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(s_layoutsMenuName))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(s_helpMenuName))
            {
                ImGui::EndMenu();
            }

            render_menu_implementation();
        }

        for (auto& child : m_childWidgets)
        {
            if (std::shared_ptr<widget> sharedChild = child.second.lock())
            {
                sharedChild->render_menu();
            }
        }
    }
}

void gluten::widget::end()
{
    ZoneScoped;

    if (m_hasStarted && !m_hasEnded)
    {
        end_implementation();
        for (auto& child : m_childWidgets)
        {
            if (std::shared_ptr<widget> sharedChild = child.second.lock())
            {
                sharedChild->end();
            }
        }
        m_hasEnded = true;
    }
}

auto gluten::widget::set_visible_in_toolbar(bool visibleInToolBar, bool defaultRender) -> void
{
    m_inToolbar = visibleInToolBar;
    m_visible   = defaultRender;
}

auto gluten::widget::set_visibile(bool visible) -> void
{
    m_visible = visible;
}

auto gluten::widget::set_children_visible(bool visible) -> void
{
    for (auto& child : m_childWidgets)
    {
        if (auto sharedChild = child.second.lock())
        {
            sharedChild->set_visibile(visible);
        }
    }
}

auto gluten::widget::get_widget_by_class(const rttr::type& type) const -> std::weak_ptr<widget>
{
    if (auto search = m_childWidgets.find(type); search != m_childWidgets.end())
    {
        return search->second;
    }
    return {};
}

auto gluten::widget::get_child_widget_count() const -> std::size_t { return m_childWidgets.size(); }

auto gluten::widget::get_widget_name() const -> std::string_view { return m_widgetName; }

auto gluten::widget::set_tick_frequency(double tickFrequency) -> void
{
    m_tickFrequency     = tickFrequency;
    m_timeSinceLastTick = 0.0;
}

void widget::render_children()
{
    ZoneScoped;

    for (auto& child : m_childWidgets)
    {
        if (std::shared_ptr<widget> sharedChild = child.second.lock())
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

auto gluten::widget::has_started() const -> bool { return m_hasStarted; }

auto gluten::widget::is_visible() const -> bool
{
    return m_visible;
}

void widget::destroy() { m_wantsDestroy = true; }

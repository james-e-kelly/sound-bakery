#pragma once

#include "gluten/pch.h"

/**
 * @def Macro to quickly create the basic constructors for a widget.
 */
#define WIDGET_CONSTRUCT(type)                                                   \
public:                                                                          \
    type(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {} \
    type(gluten::widget* parent) : gluten::widget(parent) {}

namespace gluten
{
    class widget_subsystem;

    /**
     * @brief Base widget that can render ImGui UI.
     * 
     * Widgets are owned by the @see widget_subsystem, @see root_widget, or another widget.
     * 
     * Rendering of child/owned widgets is controlled by calling the render_children function.
     * Not calling the function means no child widgets are rendered.
     * The opposite is true so rendering of child widgets can be controlled precisely.
     */
    class widget : public std::enable_shared_from_this<widget>
    {
    public:
        widget() = delete;
        widget(widget_subsystem* parentSubsystem);
        widget(widget* parentWidget);
        virtual ~widget() {}

    public:
        virtual void start();
        virtual void tick(double deltaTime) {}
        virtual void render() {}
        virtual void end() {}

        template <class T>
        requires std::derived_from<T, widget>
        [[nodiscard]] std::shared_ptr<T> add_child_widget(bool widgetOwns)
        {
            std::shared_ptr<T> ptr = std::make_shared<T>(this);
            if (widgetOwns)
            {
                m_owningChildWidgets.push_back(ptr);
            }
            m_childWidgets.push_back(ptr);
            std::shared_ptr<gluten::widget> widget = std::static_pointer_cast<gluten::widget>(ptr);
            if (m_hasStarted)
                widget->start();
            return ptr;
        }

        bool has_started() { return m_hasStarted; }

        void destroy();

    public:
        MulticastDelegate<widget*> m_onDestroy;

    protected:
        /**
         * @brief Iterates over the child widgets and renders them.
         */
        void render_children();

    protected:
        class app* get_app() const;
        widget* get_parent_widget() const;
        widget_subsystem* get_parent_subsystem() const;

    private:
        widget_subsystem* m_parentSubsystem = nullptr;
        widget* m_parentWidget              = nullptr;
        bool m_hasStarted                   = false;

        std::vector<std::weak_ptr<widget>> m_childWidgets;  //< Child widgets to iterate over
        std::vector<std::shared_ptr<widget>> m_owningChildWidgets;  //< References to widgets that are owned. Not iterated over

        friend class widget_subsystem;

        bool m_wantsDestroy = false;
    };
}  // namespace gluten
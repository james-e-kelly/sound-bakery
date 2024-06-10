#pragma once

#include "pch.h"

#define WIDGET_CONSTRUCT(type)                                          \
                                                                        \
public:                                                                 \
    type(widget_subsystem* parentSubsystem) : widget(parentSubsystem) {} \
                                                                        \
    type(widget* parent) : widget(parent) {}

class widget_subsystem;

// Base widget that can render ImGui UI
class widget
{
public:
    widget(widget_subsystem* parentSubsystem);
    widget(widget* parentWidget);
    virtual ~widget() {}

public:
    virtual void start();
    virtual void tick(double deltaTime) {}
    virtual void render() {}
    virtual void end() {}

    template <class T>
    T* add_child_widget()
    {
        m_childWidgets.push_back(std::make_unique<T>(this));
        widget* widget = m_childWidgets.back().get();
        if (m_hasStarted)
            widget->start();
        return dynamic_cast<T*>(widget);
    }

    bool has_started() { return m_hasStarted; }

    void destroy();

public:
    MulticastDelegate<widget*> m_onDestroy;

protected:
    void render_children();

protected:
    class app* get_app() const;
    widget* get_parent_widget() const;
    widget_subsystem* get_parent_subsystem() const;

private:
    widget_subsystem* m_parentSubsystem = nullptr;
    widget* m_parentWidget             = nullptr;
    bool m_hasStarted                  = false;

    std::vector<std::unique_ptr<widget>> m_childWidgets;

    friend class widget_subsystem;

    bool m_wantsDestroy = false;
};

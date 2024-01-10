#pragma once

#include "pch.h"

#define WIDGET_CONSTRUCT(type)              \
public:                                     \
    type(WidgetSubsystem* parentSubsystem)  \
    : Widget(parentSubsystem) {}            \
                                            \
    type(Widget* parent)                    \
    : Widget(parent) {}

class WidgetSubsystem;

// Base widget that can render ImGui UI
class Widget
{
public:
    Widget(WidgetSubsystem* parentSubsystem);
    Widget(Widget* parentWidget);
    virtual ~Widget() {}

public:
    virtual void Start();
    virtual void Tick(double deltaTime)  {}
    virtual void Render() {}
    virtual void End() {}
    
    template<class T>
    T* AddChildWidget()
    {
        m_childWidgets.push_back(std::make_unique<T>(this));
        Widget* widget = m_childWidgets.back().get();
        if (m_hasStarted) widget->Start();
        return dynamic_cast<T*>(widget);
    }
    
    bool HasStarted() { return m_hasStarted; }

    void Destroy();

public:
    MulticastDelegate<Widget*> m_OnDestroy;

protected:
    void RenderChildren();
    
protected:
    class App* GetApp() const;
    Widget* GetParentWidget() const;
    WidgetSubsystem* GetParentSubsystem() const;

private:
    WidgetSubsystem* m_parentSubsystem = nullptr;
    Widget* m_parentWidget = nullptr;
    bool m_hasStarted = false;

    std::vector<std::unique_ptr<Widget>> m_childWidgets;

    friend class WidgetSubsystem;

    bool m_wantsDestroy = false;
};

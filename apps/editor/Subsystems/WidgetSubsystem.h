#pragma once

#include "Subsystem.h"
#include "Widgets/Widget.h"

class WidgetSubsystem : public Subsystem
{
public:
    WidgetSubsystem(App* appOwner) : Subsystem(appOwner) {}

public:
    virtual int Init() override;
    virtual void Tick(double deltaTime) override;
    virtual void Exit() override;

public:
    template <class T>
    T* AddWidgetClass()
    {
        m_widgets.push_back(std::make_unique<T>(this));
        return dynamic_cast<T*>(m_widgets.back().get());
    }

    template <class T>
    T* AddWidgetClassToRoot()
    {
        assert(m_rootWidget);

        return m_rootWidget->AddChildWidget<T>();
    }

private:
    std::vector<std::unique_ptr<Widget>> m_widgets;
    Widget* m_rootWidget = nullptr;
};

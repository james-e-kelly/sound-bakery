#pragma once

#include "Subsystem.h"
#include "Widgets/Widget.h"

class widget_subsystem : public subsystem
{
public:
    widget_subsystem(app* appOwner) : subsystem(appOwner) {}

public:
    virtual int init() override;
    virtual void tick(double deltaTime) override;
    virtual void exit() override;

public:
    template <class T>
    T* add_widget_class()
    {
        m_widgets.push_back(std::make_unique<T>(this));
        return dynamic_cast<T*>(m_widgets.back().get());
    }

    template <class T>
    T* add_widget_class_to_root()
    {
        assert(m_rootWidget);

        return m_rootWidget->add_child_widget<T>();
    }

private:
    std::vector<std::unique_ptr<widget>> m_widgets;
    widget* m_rootWidget = nullptr;
};

#pragma once

#include "gluten/subsystems/subsystem.h"
#include "gluten/widgets/widget.h"

namespace gluten
{
    class widget_subsystem : public subsystem
    {
    public:
        widget_subsystem(app* appOwner) : subsystem(appOwner) {}

    public:
        virtual void tick(double deltaTime) override;
        virtual void exit() override;

        void set_root_widget(widget* rootWidget);
        widget* get_root_widget() const { return m_rootWidget; }

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
}  // namespace gluten
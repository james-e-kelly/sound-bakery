#pragma once

#include "gluten/subsystems/subsystem.h"
#include "gluten/widgets/widget.h"

namespace gluten
{
    /**
     * @brief Owns widgets and the root widget for all the UI.
     */
    class widget_subsystem : public subsystem
    {
    public:
        widget_subsystem(app* appOwner) : subsystem(appOwner) {}

    public:
        virtual void tick(double deltaTime) override;
        virtual void exit() override;

        void set_root_widget(widget* rootWidget);
        widget* get_root_widget() const { return m_rootWidget.get(); }

    public:
        template <class T>
        [[nodiscard]] std::shared_ptr<T> add_widget_class()
        {
            std::shared_ptr<T> ptr = std::make_shared<T>(this);
            m_widgets.push_back(ptr);
            return ptr;
        }

        /**
         * @brief Add a new widget to the root and decide whether the root widget owns the widget or the caller.
         */
        template <class T>
        [[nodiscard]] std::shared_ptr<T> add_widget_class_to_root(bool rootOwnsChild)
        {
            assert(m_rootWidget);

            return m_rootWidget->add_child_widget<T>(rootOwnsChild);
        }

    private:
        std::vector<std::weak_ptr<widget>> m_widgets;
        std::shared_ptr<widget> m_rootWidget;
    };
}  // namespace gluten
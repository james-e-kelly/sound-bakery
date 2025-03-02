#pragma once

#include "gluten/pch.h"

/**
 * @def Macro to quickly create the basic constructors for a widget.
 */
#define WIDGET_CONSTRUCT(type, name)                                                    \
                                                                                        \
public:                                                                                 \
    type(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem, name) {}  \
    type(gluten::widget* parent) : gluten::widget(parent, name) {}

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
        widget(widget_subsystem* parentSubsystem, const std::string& name);
        widget(widget* parentWidget, const std::string& name);
        virtual ~widget() {}

    public:
        auto start() -> void;
        auto tick(double deltaTime) -> void;
        auto render() -> void;
        auto render_menu() -> void;
        auto end() -> void;

        auto set_visible_in_toolbar(bool visibleInToolBar, bool defaultRender) -> void;
        auto set_visibile(bool visible) -> void;
        auto set_children_visible(bool visible) -> void;

        auto get_widget_by_class(const rttr::type& type) const -> std::weak_ptr<widget>;
        auto get_child_widget_count() const -> std::size_t;
        auto get_widget_name() const -> std::string_view;

        template <class T>
            requires std::derived_from<T, widget>
        [[nodiscard]] std::shared_ptr<T> add_child_widget(bool widgetOwns);
        
        auto has_started() const -> bool;
        auto is_visible() const -> bool;

        void destroy();

        MulticastDelegate<widget*> m_onDestroy;

        static inline const char* const s_fileMenuName = "File";
        static inline const char* const s_optionsMenuName = "Options";
        static inline const char* const s_actionsMenuName = "Actions";
        static inline const char* const s_windowsMenuName = "Windows";
        static inline const char* const s_layoutsMenuName = "Layouts";
        static inline const char* const s_helpMenuName = "Help";

    protected:
        auto virtual start_implementation() -> void {}
        auto virtual tick_implementation(double deltaTime) -> void {}
        auto virtual render_implementation() -> void {}
        auto virtual render_menu_implementation() -> void {}
        auto virtual end_implementation() -> void {}

        /**
         * @brief Iterates over the child widgets and renders them.
         */
        void render_children();

    protected:
        class app* get_app() const;
        widget* get_parent_widget() const;
        widget_subsystem* get_parent_subsystem() const;

        bool m_autoRenderChildren = true;

    private:
        widget_subsystem* m_parentSubsystem = nullptr;
        widget* m_parentWidget              = nullptr;
        bool m_hasStarted                   = false;
        bool m_hasEnded                     = false;
        std::string m_widgetName            = "Widget";
        bool m_inToolbar                    = false;
        bool m_visible                      = true;
        bool m_visibleFromToolbar           = false;

        std::map<rttr::type, std::weak_ptr<widget>> m_childWidgets; //< Child widgets to iterate over
        std::vector<std::shared_ptr<widget>> m_owningChildWidgets;  //< References to widgets that are owned. Not
                                                                    // iterated over

        friend class widget_subsystem;

        bool m_wantsDestroy = false;
    };

    template <class T>
        requires std::derived_from<T, widget>
    [[nodiscard]] std::shared_ptr<T> widget::add_child_widget(bool widgetOwns)
    {
        std::shared_ptr<T> ptr = std::make_shared<T>(this);
        if (widgetOwns)
        {
            m_owningChildWidgets.push_back(ptr);
        }
        m_childWidgets.insert({rttr::type::get<T>(), ptr});
        std::shared_ptr<gluten::widget> widget = std::static_pointer_cast<gluten::widget>(ptr);
        if (m_hasStarted)
        {
            widget->start();
        }
        return ptr;
    }
}  // namespace gluten
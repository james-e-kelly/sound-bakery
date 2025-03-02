#pragma once

#include "gluten/elements/image_button.h"
#include "gluten/widgets/widget.h"

namespace gluten
{
    /**
     * @brief Handles refreshing the dockspace.
     * 
     * Upon deletion of the object, the dockspace is submitted for the user.
     */
    struct dockspace_refresh
    {
        const ImGuiID dockspaceID = 0;
        const widget* owningWidget = nullptr;

        ImGuiID leftColumnID = 0;
        ImGuiID centerColumnID = 0;
        ImGuiID rightColumnID = 0;

        dockspace_refresh() = delete;
        dockspace_refresh(ImGuiID id, widget* widget) : dockspaceID(id), owningWidget(widget) { assert(owningWidget); }

        ~dockspace_refresh()
        { 
            ImGui::DockBuilderFinish(dockspaceID);
        }

        auto split_three_columns() -> void;

        auto assign_widget_to_node(const rttr::type& widgetType, ImGuiID idToAssignTo) -> void;
    };

    /**
     * @brief Defines a docking space layout and the widgets that should be displayed within it.
     */
    struct widget_layout
    {
        widget_layout() = default;
        widget_layout(const std::string_view& layoutName, const std::function<void(dockspace_refresh&)>& callback)
            : name(layoutName),
              onRefreshDockspace(callback)
        {}

        std::string name;
        std::function<void(dockspace_refresh&)> onRefreshDockspace; //< Should setup the layout during this callback
    };

    /**
     * @brief The base widget with titlebar and open/close icons.
     */
    class root_widget : public widget
    {
    public:
        root_widget(widget_subsystem* parentSubsystem) : widget(parentSubsystem, "Root Widget") { m_autoRenderChildren = false; }

    public:
        void start_implementation() override;
        void render_implementation() override;
        auto render_menu_implementation() -> void override;

        bool is_hovering_titlebar() { return m_hoveringTitlebar; }

        auto add_layout(const widget_layout& layout) -> void;
        auto set_layout(widget_layout& layout) -> void;
        auto get_default_layout() -> gluten::widget_layout&;

    private:
        /**
         * @brief Hides children, clears the dockspace and finishes the dockspace builder upon destruction of the returned object.
         */
        [[nodiscard]] auto refresh_dockspace() -> dockspace_refresh;

        void submit_dockspace();
        void set_root_window_to_viewport();
        void draw_titlebar();

        bool m_hoveringTitlebar = false;

        ImGuiID m_dockspaceID = 0;

        std::unique_ptr<gluten::image> m_windowIcon;
        std::unique_ptr<gluten::image_button> m_windowCloseIcon;
        std::unique_ptr<gluten::image_button> m_windowMinimiseIcon;
        std::unique_ptr<gluten::image_button> m_windowMaximiseIcon;
        std::unique_ptr<gluten::image_button> m_windowRestoreIcon;

        std::vector<widget_layout> m_layouts;
    };
}  // namespace gluten
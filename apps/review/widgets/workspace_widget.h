#pragma once

#include "pch.h"
#include "data/review_data.h"
#include "gluten/widgets/window_widget.h"
#include "gluten/widgets/popup_widget.h"

class create_project_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(create_project_popup, "Create Project", gluten::popup_widget)

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char projectNameBuffer[textBufferSize] = {0};
    char projectDescriptionBuffer[textBufferSize] = {0};
};

class create_review_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(create_review_popup, "Create Review", gluten::popup_widget)

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char reviewNameBuffer[textBufferSize]        = {0};
    char reviewDescriptionBuffer[textBufferSize] = {0};
    char reviewUrlBuffer[textBufferSize] = {0};
    new_review_data m_reviewData;
};

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;
    virtual auto render_menu_implementation() -> void override;

private:
    auto render_content() -> void;
    auto render_list() -> void;
    auto render_left_toolbar() -> void;

    enum active_view
    {
        reviews_view,
        users_view
    };

    active_view m_activeView = reviews_view;
};

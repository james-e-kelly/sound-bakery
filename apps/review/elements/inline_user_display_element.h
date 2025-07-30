#pragma once

#include "pch.h"

#include "gluten/elements/image.h"

class inline_user_avatar_element : public gluten::element
{
public:
    inline_user_avatar_element(const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userEmailAddress(userEmail) {}

    auto set_avatar_render(gluten::image_render render) -> void;

protected:
    auto render_element(const ImRect& parentRect) -> bool override;

private:
    gluten::image_render m_render = gluten::image_render::circular;
    std::string m_userEmailAddress;
};

class logged_in_user_element : public gluten::element
{
public:
    logged_in_user_element(const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userEmailAddress(userEmail) {}

protected:
    auto render_element(const ImRect& parentRect) -> bool override;

private:
    std::string m_userEmailAddress;
};

class inline_user_display_element : public gluten::element
{
public:
    inline_user_display_element() : element(element::anchor_preset::stretch_full) {}
    inline_user_display_element(const std::string& userName, const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userDisplayName(userName), m_userEmailAddress(userEmail) {}

protected:
    auto render_element(const ImRect& parentRect) -> bool override;

private:
    std::string m_userEmailAddress;
    std::string m_userDisplayName;
};
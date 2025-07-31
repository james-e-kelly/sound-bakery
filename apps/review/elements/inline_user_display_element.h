#pragma once

#include "pch.h"

#include "gluten/elements/image.h"
#include "data/review_data.h"

class user_avatar_element : public gluten::element
{
public:
    user_avatar_element(const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userEmailAddress(userEmail) {}

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

class reviewer_display_element : public gluten::element
{
public:
    reviewer_display_element() = delete;
    reviewer_display_element(const reviewer_data& reviewUser)
        : element(element::anchor_preset::stretch_full),
          m_userDisplayName(reviewUser.m_displayName),
          m_userEmailAddress(reviewUser.m_email),
          m_vote(reviewUser.m_vote)
    {
    }

protected:
    auto render_element(const ImRect& parentRect) -> bool override;

private:
    std::string m_userEmailAddress;
    std::string m_userDisplayName;
    review_vote m_vote = review_vote::no_vote;
};
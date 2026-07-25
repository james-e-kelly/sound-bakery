#pragma once

#include "pch.h"

#include "data/review_data.h"
#include "data/user_settings_data.h"
#include "gluten/elements/image.h"

class user_avatar_element : public gluten::element
{
public:
    user_avatar_element(const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userEmailAddress(userEmail) {}

    auto set_avatar_render(gluten::image_render render) -> void;

    auto get_image_rect() const -> ImRect
    {
        return avatarImage ? avatarImage->get_element_rect() : get_element_rect();
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    gluten::image_render m_render = gluten::image_render::circular;
    std::string m_userEmailAddress;
    gluten::image* avatarImage = nullptr;
};

class logged_in_user_element : public gluten::element
{
public:
    logged_in_user_element(const std::string& userEmail) : element(element::anchor_preset::stretch_full), m_userEmailAddress(userEmail) {}

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    std::string m_userEmailAddress;
};

class reviewer_display_element : public gluten::element
{
public:
    reviewer_display_element() = delete;
    reviewer_display_element(const reviewer_data& reviewUser, int64_t reviewId)
        : element(element::anchor_preset::stretch_full),
          m_userDisplayName(reviewUser.m_displayName),
          m_userEmailAddress(reviewUser.m_email),
          m_vote(reviewUser.m_vote),
          m_reviewId(reviewId),
          m_userId(reviewUser.m_userId)
    {
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    std::string m_userEmailAddress;
    std::string m_userDisplayName;
    review_vote m_vote = review_vote::no_vote;

    int64_t m_userId   = -1;
    int64_t m_reviewId = -1;

    gluten::data_source<user_settings_data> m_userSettings;
};
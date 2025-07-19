#pragma once

#include "pch.h"

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
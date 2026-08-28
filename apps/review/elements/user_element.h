#include "pch.h"

#include "elements/list_element.h"

class user_element : public list_element
{
public:
    user_element() = delete;
    user_element(const user_data& user)
        : list_element
        (
            user.m_displayName,
            user.m_title,
            user.m_privileges == user_privileges::admin ? ICON_LC_USER_COG : user.m_privileges == user_privileges::user ? ICON_LC_USER_PEN : ICON_LC_USER
        ),
          m_user(user)
    {
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override
    {
        gluten::imgui::scoped_id id(ImGui::GetID(m_user.m_email.c_str()));

        return list_element::render_element(renderInfo);
    }

private:
    const user_data& m_user;
};
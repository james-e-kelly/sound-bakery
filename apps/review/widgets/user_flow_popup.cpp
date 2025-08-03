#include "user_flow_popup.h"

#include "managers/workspace_manager.h"

#include "gluten/widgets/popup_widget.h"
#include "sodium.h"

auto user_flow_popup::start_implementation() -> void
{
    sodium_mlock(m_passwordBuffer, g_rawPasswordSize);
}

auto user_flow_popup::end_implementation() -> void
{
    sodium_munlock(m_passwordBuffer, g_rawPasswordSize);
}

auto user_flow_popup::set_flow_type(user_flow_type type) -> void { m_type = type; }

auto user_flow_popup::render_popup() -> void
{
    std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>();

    if (!workspaceManager)
    {
        return;
    }

    const auto& allUsers = workspaceManager->get_all_users();

    switch (allUsers.m_state)
    {
        case gluten::cache_state::loading:
            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(gluten::loading_popup::s_progressBarWidth, 0.0f), "Waiting for users to load...");
            return;
            break;
        default:
            m_firstUserCreation = allUsers.m_cache.empty();
            break;
    }

    if (m_firstUserCreation)
    {
        m_privileges = user_privileges::admin;
    }

    if (ImGui::InputTextWithHint("Email", "email@domain.com", m_emailBuffer, textBufferSize))
    {
        m_errorText.clear();
    }

    if (ImGui::InputText("Password", m_passwordBuffer, g_rawPasswordSize, ImGuiInputTextFlags_Password))
    {
        m_errorText.clear();
    }

    if (m_type != user_flow_type::login_user)
    {
        ImGui::Separator();

        ImGui::InputTextWithHint("Display Name", "", m_displayNameBuffer, textBufferSize);
        ImGui::InputTextWithHint("Job Title", "", m_titleBuffer, textBufferSize);

        ImGui::BeginDisabled(m_firstUserCreation);

        if (ImGui::BeginCombo("Role", get_user_privileges_string(m_privileges).c_str()))
        {
            if (ImGui::Selectable("Guest"))
            {
                m_privileges = user_privileges::guest;
            }
            
            const bool loggedIn = !m_userSettings->m_loggedInUser.m_sessionToken.empty();
            const bool canCreateUsers = loggedIn && m_userSettings->m_loggedInUser.m_privileges == user_privileges::admin;

            if (canCreateUsers)
            {
                if (ImGui::Selectable("User"))
                {
                    m_privileges = user_privileges::user;
                }
            }

            if (m_firstUserCreation || canCreateUsers)
            {
                if (ImGui::Selectable("Admin"))
                {
                    m_privileges = user_privileges::admin;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::EndDisabled();
    }

    const bool emailIsFilled = m_emailBuffer[0] != '\0';
    const bool passwordIsFilled = m_passwordBuffer[0] != '\0';
    const bool displayNameIsFilled = m_displayNameBuffer[0] != '\0';
    const bool titleIsFilled = m_titleBuffer[0] != '\0';

    const bool detailsAreValid = emailIsFilled && passwordIsFilled;
    const bool extraDetailsAreValid = displayNameIsFilled && titleIsFilled;

    switch (m_type)
    {
        case user_flow_type::login_user:
            ImGui::BeginDisabled(!detailsAreValid);
            break;
        case user_flow_type::new_user:
        case user_flow_type::new_user_and_login:
            ImGui::BeginDisabled(!detailsAreValid || !extraDetailsAreValid);
            break;
        default:
            break;
    }

    ImGui::BeginDisabled(static_cast<bool>(m_loginResult));

    switch (m_type)
    {
        case user_flow_type::login_user:
            if (ImGui::Button("Login"))
            {
                m_errorText.clear();

                login_request_data loginRequest;
                loginRequest.m_email = m_emailBuffer;
                std::memcpy(loginRequest.m_rawPassword.data(), m_passwordBuffer, g_rawPasswordSize);

                m_loginResult = workspaceManager->login_user(loginRequest);
            }
            break;
        case user_flow_type::new_user:
            if (ImGui::Button("Create"))
            {
                m_errorText.clear();

                new_user_data newUser;
                newUser.m_displayName = m_displayNameBuffer;
                newUser.m_email       = m_emailBuffer;
                std::memcpy(newUser.m_rawPassword.data(), m_passwordBuffer, g_rawPasswordSize);
                newUser.m_title               = m_titleBuffer;
                newUser.m_requestedPrivileges = m_privileges;

                m_loginResult = workspaceManager->create_user(newUser, m_userSettings->m_loggedInUser.m_sessionToken);
            }
            break;
        case user_flow_type::new_user_and_login:
            if (ImGui::Button("Create"))
            {
                m_errorText.clear();

                new_user_data newUser;
                newUser.m_displayName = m_displayNameBuffer;
                newUser.m_email       = m_emailBuffer;
                std::memcpy(newUser.m_rawPassword.data(), m_passwordBuffer, g_rawPasswordSize);
                newUser.m_title               = m_titleBuffer;
                newUser.m_requestedPrivileges = m_privileges;

                m_loginResult = workspaceManager->create_user_and_login(newUser);
            }
            break;
        default:
            break;
    }

    ImGui::EndDisabled();
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    if (m_loginResult)
    {
        ImGui::SameLine();

        ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);

        switch (m_loginResult.status())
        {
            case concurrencpp::result_status::value:
            {
                const auto loginValue = m_loginResult.get();
                if (!loginValue.has_value())
                {
                    m_errorText = loginValue.error().m_errorMessage;
                }
                break;
            }
            case concurrencpp::result_status::exception:
                m_errorText = "Expection occurred";
                break;
            default:
                break;
        }
    }

    if (!m_errorText.empty())
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(m_errorText.c_str());
    }
}
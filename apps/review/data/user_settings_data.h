#pragma once

#include "pch.h"

#include "data/user_data.h"

enum class review_app_theme
{
    dark,
    light
};

/**
 * @brief Cached user settings like the open workspace
 *
 * Stores where the workspace lives so it can be loaded upon boot automatically.
 */
struct user_settings_data
{
    std::filesystem::path m_workspaceFilePath;  //< Top most file that will be used to find reviews and data in
                                                // subfolders
    std::string m_selectedProjectName;

    logged_in_user_data m_loggedInUser;

    review_app_theme m_theme = review_app_theme::dark;
    bool m_displayPhases     = true;
    bool m_displayQuality    = true;
    bool m_displayScrutiny   = true;

    auto workspace_exists() const -> bool
    {
        return std::filesystem::exists(m_workspaceFilePath);
    }

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& boost::serialization::make_nvp("Workspace", m_workspaceFilePath);

        if (version >= review_app_save_selected_project)
        {
            archive & boost::serialization::make_nvp("SelectedProject", m_selectedProjectName);
        }

        if (version >= review_app_user_tokens)
        {
            archive & boost::serialization::make_nvp("UserLogin", m_loggedInUser);
        }

        if (version >= review_app_first_settings)
        {
            archive & boost::serialization::make_nvp("theme", m_theme);
            archive & boost::serialization::make_nvp("display_phases", m_displayPhases);
            archive & boost::serialization::make_nvp("display_quality", m_displayQuality);
            archive & boost::serialization::make_nvp("display_scrutiny", m_displayScrutiny);
        }
    }
};

BOOST_CLASS_VERSION(user_settings_data, review_app_version_current)
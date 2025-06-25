#include "workspace_data.h"

namespace
{
    constexpr const char* g_projectDirectoryName = "Projects";
    constexpr const char* g_userDirectoryName    = "Users";
}

auto workspace_controller::workspace_exists() const -> bool 
{
    return std::filesystem::exists(get_const_data()->m_workspaceFilePath);
}

auto workspace_controller::get_projects_directory() const -> std::filesystem::path
{
    return get_const_data()->m_workspaceFilePath / g_projectDirectoryName;
}

auto workspace_controller::get_user_directory() const -> std::filesystem::path
{
    return get_const_data()->m_workspaceFilePath / g_userDirectoryName;
}
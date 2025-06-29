#pragma once

#include "pch.h"

#include "gluten/data/data_source.h"

#include "data/workspace_data.h"
#include "data/project_data.h"
#include "data/user_settings_data.h"

class intro_widget;
class workspace_widget;

class workspace_manager : public gluten::manager	
{
public:
    workspace_manager(gluten::app* app) : gluten::manager(app) {}

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;
    auto create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void;
    auto close_workspace() -> void;

    auto add_existing_project(const project_data& projectData) -> void;
    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;
    auto select_project(const std::string& projectName) -> void;
    [[nodiscard]] auto has_selected_project() const -> bool;

    [[nodiscard]] auto get_selected_project() const -> std::shared_ptr<project_data>;
    [[nodiscard]] auto get_projects() const -> std::vector<std::shared_ptr<project_data>>;
    
    [[nodiscard]] auto get_workspace_file() const -> std::filesystem::path;
    [[nodiscard]] auto get_workspace_directory() const -> std::filesystem::path;
    [[nodiscard]] auto get_projects_directory() const -> std::filesystem::path;
    [[nodiscard]] auto get_user_directory() const -> std::filesystem::path;

protected:
    auto init(gluten::app* app) -> void override;
    auto start() -> void override;

private:
    static auto file_is_workspace(const std::filesystem::path& file) -> bool;

    auto load_projects_from_workspace() -> void;

    std::shared_ptr<intro_widget> m_introWidget;
    std::shared_ptr<workspace_widget> m_workspaceWidget;

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::vector<std::shared_ptr<project_data>> m_projects;
    std::shared_ptr<project_data> m_selectedProject;
};
#pragma once

#include "pch.h"

#include "data/workspace_data.h"
#include "data/project_data.h"

class intro_widget;
class workspace_widget;

class workspace_manager : public gluten::manager	
{
public:
    workspace_manager(gluten::app* app) : gluten::manager(app) {}

    virtual auto init(gluten::app* app) -> void override;
    virtual auto start() -> void override;

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;
    auto create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void;
    auto close_workspace() -> void;

    auto add_existing_project(const project_data& projectData) -> void;
    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;

    auto get_projects() const -> std::vector<std::shared_ptr<project_data>>;

private:
    workspace_cache_controller m_workspaceController;

    std::shared_ptr<intro_widget> m_introWidget;
    std::shared_ptr<workspace_widget> m_workspaceWidget;

    std::vector<std::shared_ptr<project_data>> m_projects;
};
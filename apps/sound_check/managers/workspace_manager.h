#pragma once

#include "pch.h"

#include "data/workspace_data.h"

class intro_widget;
class workspace_widget;

enum workspace_file_data_version
{
    workspace_file_version_start = 0,

    // Add versions above this //
    workspace_file_version_end,
    workspace_file_version_current = workspace_file_version_end - 1
};

struct workspace_file_data
{
    std::string m_workspaceName;

    template<class archive_class>
    auto serialize(archive_class& archive, unsigned int fileVersion)
    {
        archive & boost::serialization::make_nvp("name", m_workspaceName);
    }
};

BOOST_CLASS_VERSION(workspace_file_data, workspace_file_version_current)

class workspace_manager : public gluten::manager	
{
public:
    workspace_manager(gluten::app* app) : gluten::manager(app) {}

    virtual auto init(gluten::app* app) -> void override;
    virtual auto start() -> void override;

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;
    auto create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void;
    auto close_workspace() -> void;

private:
    workspace_controller m_workspaceController;

    std::shared_ptr<intro_widget> m_introWidget;
    std::shared_ptr<workspace_widget> m_workspaceWidget;
};
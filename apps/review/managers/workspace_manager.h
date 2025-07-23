#pragma once

#include "pch.h"

#include "gluten/data/data_source.h"

#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/review_data.h"
#include "data/workspace_data.h"
#include "data/project_data.h"
#include "data/user_settings_data.h"

class intro_widget;
class review_database;
class workspace_widget;

class workspace_manager : public gluten::manager	
{
public:
    workspace_manager(gluten::app* app) : gluten::manager(app) {}
    ~workspace_manager();

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;
    auto create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void;
    auto close_workspace() -> void;

    auto add_existing_project(const project_data& projectData) -> void;
    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;
    auto select_project(const std::string& projectName) -> void;
    [[nodiscard]] auto has_selected_project() const -> bool;

    [[nodiscard]] auto get_selected_project() const -> const project_data&;
    [[nodiscard]] auto get_projects() const -> const std::set<project_data>&;
    
    [[nodiscard]] auto get_workspace_name() const -> std::string;
    [[nodiscard]] auto get_workspace_file() const -> std::filesystem::path;
    [[nodiscard]] auto get_workspace_directory() const -> std::filesystem::path;

    auto select_review(int64_t reviewId) -> void;
    auto get_selected_review() const -> const review_data&;
    auto create_review(const new_review_data& newReview) -> void;
    auto update_review(const review_data& updatedReview) -> void;
    auto get_all_reviews() const -> const std::set<review_data>&;

    // Activity
    auto get_all_activity_for_review(int64_t reviewId) const -> const std::vector<activity_data>&;
    auto get_activity_count_for_review(int64_t reviewId) const -> std::size_t;

    // Comments
    auto get_all_comments_for_review(int64_t reviewId) const -> const std::vector<comment_data>&;
    auto get_comments_count_for_review(int64_t reviewId) const -> std::size_t;

protected:
    auto init(gluten::app* app) -> void override;
    auto start() -> void override;

private:
    static auto file_is_workspace(const std::filesystem::path& file) -> bool;

    auto load_projects_from_workspace() -> void;

    std::shared_ptr<intro_widget> m_introWidget;
    std::shared_ptr<workspace_widget> m_workspaceWidget;

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::set<project_data> m_projects;
    std::set<review_data> m_reviews;
    project_data m_selectedProject;
    review_data m_selectedReview;
    std::shared_ptr<review_database> m_database;
};
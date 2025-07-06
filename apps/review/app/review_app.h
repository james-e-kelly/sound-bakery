#pragma once

#include "gluten/app/app.h"

class workspace_manager;
class review_app_drop_target;
class review_database;

static inline constexpr const char* g_externalFileDragDropType = "FILE_DRAG_DROP";

class review_app final : public gluten::app
{
public:
    friend review_app_drop_target;

    auto get_is_drag_dropping() const -> bool
    {
        return m_isDragDropping;
    }

    auto get_drag_drop_files() -> std::unordered_set<std::filesystem::path>;
    
    static auto get() -> review_app*;

    auto get_database_thread_executor() const -> std::shared_ptr<concurrencpp::worker_thread_executor>
    {
        return m_databaseThread;
    }

protected:
    auto pre_init() -> void override;
    auto post_init() -> void override;
    auto exit() -> void override;
    auto tick_implementation() -> void override;

private:
    auto set_is_drag_dropping(bool dragDropping) -> void
    {
        m_isDragDropping = dragDropping;
    }

    std::shared_ptr<workspace_manager> m_workspaceManager;
    std::unique_ptr<review_app_drop_target> m_dropTarget;

    bool m_isDragDropping   = false;
    bool m_isDragDropReady  = false;

    std::shared_ptr<concurrencpp::worker_thread_executor> m_databaseThread;
};
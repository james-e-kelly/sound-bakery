#pragma once

#include "gluten/app/app.h"

namespace gluten
{
    class audio_subsystem;
}
class video_subsystem;
class workspace_manager;
class review_app_drop_target;
class review_database;

static inline constexpr const char* g_externalFileDragDropType = "FILE_DRAG_DROP";

class review_app final : public gluten::app
{
public:
    friend review_app_drop_target;

    auto get_is_drag_dropping() const -> bool;
    auto get_drag_drop_files() const -> std::unordered_set<std::filesystem::path>;
    auto get_database_thread_executor() const -> std::shared_ptr<concurrencpp::worker_thread_executor>;

    static auto get() -> review_app*;
    static auto get_review_database() -> std::shared_ptr<review_database>;
    static auto create_review_database(const std::filesystem::path& path) -> void;
    static auto close_review_database() -> void;
    static auto setup_client(const std::string& serverAddress) -> void;
    static auto setup_server(const std::filesystem::path& workspaceFile) -> void;
    static auto reset_to_intro() -> void;

protected:
    auto pre_init() -> void override;
    auto post_init() -> void override;
    auto exit() -> void override;
    auto tick_implementation() -> void override;

private:
    auto set_is_drag_dropping(bool dragDropping) -> void;

    std::shared_ptr<gluten::audio_subsystem> m_audioSubsystem;
    std::shared_ptr<video_subsystem> m_videoSubsystem;
    std::unique_ptr<review_app_drop_target> m_dropTarget;

    mutable bool m_isDragDropping   = false;
    mutable bool m_isDragDropReady  = false;

    std::shared_ptr<concurrencpp::worker_thread_executor> m_databaseThread;

    std::shared_ptr<review_database> m_database;
};
#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/editor/project/project_configuration.h"

namespace sbk::engine
{
    class sound_container;
}

namespace sbk::editor
{
    /**
     * @brief Manages a project file and the objects contained within it.
     *
     * The project class also creates a sound container to play sound files on.
     */
    class SB_CLASS project : public sbk::core::object_owner
    {
    public:
        auto open_project(const std::filesystem::path& projectFile) -> concurrencpp::result<bool>;
        void create_project(const std::filesystem::path& projectFile) {}

        void save_project() const;

        auto encode_all_media() const -> concurrencpp::result<void>;
        auto build_soundbanks() -> concurrencpp::result<void>;

        [[nodiscard]] const project_configuration& get_config() const;
        [[nodiscard]] std::weak_ptr<sbk::engine::sound_container> get_preview_container() const;

    private:
        auto load_sounds() -> concurrencpp::result<void>;
        auto load_system() -> concurrencpp::result<void>;
        auto load_objects() -> concurrencpp::result<void>;

        static auto load_single_sound(sbk::editor::project* project, std::filesystem::path filePath) -> concurrencpp::result<void>;
        static auto load_single_object(sbk::editor::project* project, std::filesystem::path filePath) -> concurrencpp::result<void>;

        auto create_preview_container() -> void;

        auto save_system() const -> concurrencpp::result<void>;
        auto save_objects() const -> concurrencpp::result<void>;

        project_configuration m_projectConfig;
        std::weak_ptr<sbk::engine::sound_container> m_previewSoundContainer;

        mutable concurrencpp::async_lock m_projectLock;
    };
}  // namespace sbk::editor
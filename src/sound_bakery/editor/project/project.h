#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/error/error.h"
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
        auto open_project(const std::filesystem::path& projectFile) -> sbk::result<void>;
        auto create_project(const std::filesystem::path& projectFile) -> void {}

        auto save_project() const -> sbk::result<void>;

        auto encode_all_media() const -> void;
        auto build_soundbanks() -> sbk::result<void>;

        [[nodiscard]] auto get_config() const -> const project_configuration&;
        [[nodiscard]] auto get_preview_container() const -> std::weak_ptr<sbk::engine::sound_container>;

    private:
        auto load_sounds() -> sbk::result<void>;
        auto load_system() -> sbk::result<void>;
        auto load_objects() -> sbk::result<void>;

        auto create_preview_container() -> sbk::result<void>;

        auto save_system() const -> sbk::result<void>;
        auto save_objects() const -> sbk::result<void>;

        project_configuration m_projectConfig;
        std::weak_ptr<sbk::engine::sound_container> m_previewSoundContainer;
    };
}  // namespace sbk::editor
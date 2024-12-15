#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/editor/project/project_configuration.h"

#include "yaml-cpp/yaml.h"

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
        bool open_project(const std::filesystem::path& projectFile);
        void create_project(const std::filesystem::path& projectFile) {}

        void save_project() const;

        void encode_all_media() const;

        [[nodiscard]] const project_configuration& get_config() const;
        [[nodiscard]] std::weak_ptr<sbk::engine::sound_container> get_preview_container() const;

    private:
        void loadSounds();
        void loadSystem();
        void loadObjects();

        void createPreviewContainer();

        void build_soundbanks() const;

        void saveSystem() const;
        void saveObjects() const;
        void saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const;

        project_configuration m_projectConfig;
        std::weak_ptr<sbk::engine::sound_container> m_previewSoundContainer;
    };
}  // namespace sbk::editor
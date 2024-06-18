#pragma once

#include "sound_bakery/core/core_include.h"

#include "yaml-cpp/yaml.h"

namespace sbk::engine
{
    class SoundContainer;
}

namespace sbk::editor
{
    /**
     * @brief Handles file and folder paths for a project.
     */
    struct ProjectConfiguration
    {
        ProjectConfiguration() = default;

        ProjectConfiguration(const std::filesystem::path& projectFile)
            : m_projectFile(projectFile),
              m_projectFolder(projectFile.parent_path()),
              m_projectName(projectFile.filename().stem().string())
        {
        }

        std::filesystem::path m_projectFile;

        std::string m_projectName;
        std::filesystem::path m_projectFolder;

        std::filesystem::path sourceFolder() const { return m_projectFolder / "Source"; }
        std::filesystem::path objectFolder() const { return m_projectFolder / "Objects"; }
        std::filesystem::path buildFolder() const { return m_projectFolder / "Build"; }
        std::filesystem::path savedFolder() const { return m_projectFolder / "Saved"; }

        std::filesystem::path encodedFolder() const { return buildFolder() / "Encoded"; }
        std::filesystem::path logFolder() const { return savedFolder() / "Logs"; }

        /**
         * @brief Converts an object type to a folder location
         */
        std::filesystem::path typeFolder(const rttr::type& type) const;

        std::string getIdFilename(sbk::core::database_object* databaseObject,
                                  std::optional<std::string> extensionOverride = std::nullopt) const
        {
            return std::to_string(databaseObject->get_database_id()) +
                   (extensionOverride.has_value() ? extensionOverride.value() : ".yaml");
        }

        bool isValid() const { return std::filesystem::exists(m_projectFile); }
    };

    /**
     * @brief Manages a project file and the objects contained within it.
     */
    class SB_CLASS project
    {
    public:
        bool openProject(const std::filesystem::path& projectFile);
        void createProject(const std::filesystem::path& projectFile) {}

        void saveProject() const;

        void encodeAllMedia() const;

        [[nodiscard]] const ProjectConfiguration& getConfig() const { return m_projectConfig; }

        [[nodiscard]] sbk::engine::SoundContainer* getPreviewContainer() const { return m_previewSoundContainer; }

    private:
        void loadSounds();
        void loadSystem();
        void loadObjects();

        void createPreviewContainer();

        void buildSoundbanks() const;

        void saveSystem() const;
        void saveObjects() const;
        void saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const;

    private:
        ProjectConfiguration m_projectConfig;
        sbk::engine::SoundContainer* m_previewSoundContainer;
    };
}  // namespace sbk::editor
#pragma once

#include "sound_bakery/core/core_include.h"

#include "yaml-cpp/yaml.h"

namespace SB::Editor
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
            m_projectName(projectFile.filename().string())
        {}

        std::filesystem::path m_projectFile;

        std::string m_projectName;
        std::filesystem::path m_projectFolder;

        std::filesystem::path sourceFolder() const { return m_projectFolder / "Source"; }
        std::filesystem::path objectFolder() const { return m_projectFolder / "Objects"; }
        std::filesystem::path buildFolder() const { return m_projectFolder / "Build"; }

        /**
         * @brief Converts an object type to a folder location
        */
        std::filesystem::path typeFolder(const rttr::type& type) const;

        std::string getIdFilename(SB::Core::DatabaseObject* databaseObject, std::optional<std::string> extensionOverride = std::nullopt) const
        {
            return std::to_string(databaseObject->getDatabaseID()) + (extensionOverride.has_value() ? extensionOverride.value() : ".yaml");
        }

        bool isValid() const { return std::filesystem::exists(m_projectFile); }
    };

    /**
     * Manages a project file and the objects contained within it.
     */
    class SB_CLASS Project
    {
    public:
        bool openProject(const std::filesystem::path& projectFile);
        void createProject(const std::filesystem::path& projectFile) {}

        void saveProject() const;

        const ProjectConfiguration& getConfig() const { return m_projectConfig; }

    private:
        void loadSounds();
        void loadSystem();
        void loadObjects();

        void buildSoundbanks() const;

        void saveSystem() const;
        void saveObjects() const;
        void saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const;

    private:
        ProjectConfiguration m_projectConfig;
    };
}  // namespace SB::Editor
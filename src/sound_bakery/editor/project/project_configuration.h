#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::editor
{
    /**
     * @brief Handles file and folder paths for a project.
     */
    struct project_configuration
    {
        project_configuration() = default;

        /**
         * @brief Opens an project existing project file.
         */
        project_configuration(const std::filesystem::path& projectFile);

        /**
         * @brief Creates a new project inside the chosen directory.
         */
        project_configuration(const std::filesystem::directory_entry& projectDirectory, const std::string& projectName);

        static constexpr std::string_view projectExtension        = "bakery";
        static constexpr std::string_view projectExtensionWithDot = ".bakery";

        [[nodiscard]] std::filesystem::path source_folder() const { return m_projectFolder / "Source"; }
        [[nodiscard]] std::filesystem::path object_folder() const { return m_projectFolder / "Objects"; }
        [[nodiscard]] std::filesystem::path build_folder() const { return m_projectFolder / "Build"; }
        [[nodiscard]] std::filesystem::path saved_folder() const { return m_projectFolder / "Saved"; }
        [[nodiscard]] std::filesystem::path encoded_folder() const { return build_folder() / "Encoded"; }
        [[nodiscard]] std::filesystem::path log_folder() const { return saved_folder() / "Logs"; }

        [[nodiscard]] std::filesystem::path project_file() const { return m_projectFile; }
        [[nodiscard]] std::filesystem::path project_folder() const { return m_projectFolder; }
        [[nodiscard]] std::string_view project_name() const { return m_projectName; }

        [[nodiscard]] std::filesystem::path type_folder(const rttr::type& type) const;  //< Converts an object type to a
                                                                                        // folder location

        [[nodiscard]] static std::string get_filename_for_id(
            sbk::core::database_object* databaseObject, std::optional<std::string> extensionOverride = std::nullopt);

        [[nodiscard]] bool is_valid() const;  //< Returns true if the project file exists

    private:
        std::filesystem::path m_projectFile;
        std::string m_projectName;
        std::filesystem::path m_projectFolder;
    };
}  // namespace sbk::editor
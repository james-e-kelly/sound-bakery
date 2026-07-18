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
        project_configuration(const std::filesystem::directory_entry& projectDirectory, std::string_view projectName);

        static constexpr std::string_view projectExtension           = "bakery";
        static constexpr std::string_view projectExtensionWithDot    = ".bakery";
        static constexpr std::string_view outputBankExtension        = "bnk";
        static constexpr std::string_view outputBankExtensionWithDot = ".bnk";
        static constexpr std::string_view initBankName               = "Init";

        [[nodiscard]] auto source_folder() const -> std::filesystem::path { return m_projectFolder / "Source"; }
        [[nodiscard]] auto object_folder() const -> std::filesystem::path { return m_projectFolder / "Objects"; }
        [[nodiscard]] auto build_folder() const -> std::filesystem::path { return m_projectFolder / "Build"; }
        [[nodiscard]] auto saved_folder() const -> std::filesystem::path { return m_projectFolder / "Saved"; }
        [[nodiscard]] auto encoded_folder() const -> std::filesystem::path { return build_folder() / "Encoded"; }
        [[nodiscard]] auto log_folder() const -> std::filesystem::path { return saved_folder() / "Logs"; }
        [[nodiscard]] auto plugin_folder() const -> std::filesystem::path { return m_projectFolder / "Plugins"; }

        [[nodiscard]] auto project_file() const -> std::filesystem::path { return m_projectFile; }
        [[nodiscard]] auto project_folder() const -> std::filesystem::path { return m_projectFolder; }
        [[nodiscard]] auto project_name() const -> std::string_view { return m_projectName; }

        [[nodiscard]] auto type_folder(const rttr::type& type) const -> std::filesystem::path;  //< Converts an object type to a
                                                                                        // folder location

        [[nodiscard]] static auto get_filename_for_id(
            sbk::core::database_object* databaseObject, std::optional<std::string> extensionOverride = std::nullopt) -> std::string;

        [[nodiscard]] auto is_valid() const -> bool;  //< Returns true if the project file exists

    private:
        std::filesystem::path m_projectFile;
        std::string m_projectName;
        std::filesystem::path m_projectFolder;
    };
}  // namespace sbk::editor
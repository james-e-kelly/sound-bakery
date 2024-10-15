#include "project_configuration.h"

sbk::editor::project_configuration::project_configuration(const std::filesystem::path& projectFile)
    : m_projectFile(projectFile),
      m_projectName(projectFile.filename().stem().string()),
      m_projectFolder(projectFile.parent_path())
{
}

sbk::editor::project_configuration::project_configuration(const std::filesystem::directory_entry& projectDirectory,
                                                          const std::string& projectName)
    : m_projectFile(projectDirectory.path() / (projectName + projectExtensionWithDot.data())),
      m_projectName(projectName),
      m_projectFolder(projectDirectory.path())
{
    const bool projectDoesNotExist = !is_valid();
    const bool folderExists        = std::filesystem::exists(m_projectFolder);
    const bool folderIsEmpty       = std::filesystem::is_empty(m_projectFolder);

    if (projectDoesNotExist && folderExists && folderIsEmpty)
    {
        YAML::Emitter projectYAML;

        projectYAML << YAML::BeginDoc;
        projectYAML << YAML::BeginMap;
        projectYAML << YAML::Key << "Sound Bakery Version" << YAML::Value << SBK_VERSION_STRING;
        projectYAML << YAML::EndMap;
        projectYAML << YAML::EndDoc;

        {
            std::ofstream outputStream(m_projectFile.c_str(), std::ios_base::trunc);
            outputStream << projectYAML.c_str();
        }

        std::filesystem::create_directory(source_folder());
        std::filesystem::create_directory(object_folder());
        std::filesystem::create_directory(build_folder());
        std::filesystem::create_directory(saved_folder());
        std::filesystem::create_directory(encoded_folder());
        std::filesystem::create_directory(log_folder());
    }
}

std::filesystem::path sbk::editor::project_configuration::type_folder(const rttr::type& type) const
{
    std::filesystem::path rootObjectFolder = object_folder();

    if (!type.is_valid())
    {
        return rootObjectFolder;
    }

    const rttr::string_view typeName = type.get_name();
    const std::string typeNameString = typeName.to_string();

    const std::size_t lastColonCharacterPos = typeNameString.find_last_of(':') + 1;

    if (lastColonCharacterPos == std::string::npos || lastColonCharacterPos >= typeNameString.size())
    {
        return rootObjectFolder;
    }

    return rootObjectFolder / typeNameString.substr(lastColonCharacterPos, std::string::npos);
}

std::string sbk::editor::project_configuration::get_filename_for_id(sbk::core::database_object* databaseObject,
                                                                    std::optional<std::string> extensionOverride)
{
    return std::to_string(databaseObject->get_database_id()) +
           (extensionOverride.has_value() ? extensionOverride.value() : ".yaml");
}

bool sbk::editor::project_configuration::is_valid() const { return std::filesystem::exists(m_projectFile); }

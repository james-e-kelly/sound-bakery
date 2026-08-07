#include "app.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/sound/sound.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "widgets/root_widget.h"

namespace path_helpers
{
    static const char* ResourcesFolder = "Resources";
}

namespace editor_app_cli_arguments
{
    static constexpr const char* s_projectFile = "project";
}  // namespace editor_app_cli_arguments

gluten::app* create_application() { return new editor_app(); }

void editor_app::open_project(const std::filesystem::path& project_file)
{
    remove_manager_by_class<project_manager>();
    if (std::shared_ptr<project_manager> projectManager = add_manager_class<project_manager>())
    {
        projectManager->init_project(project_file);
    }
}

void editor_app::create_and_open_project(const std::filesystem::directory_entry& projectFolder, const std::string_view& projectName)
{
    remove_manager_by_class<project_manager>();
    if (std::shared_ptr<project_manager> projectManager = add_manager_class<project_manager>())
    {
        projectManager->create_project(projectFolder, projectName.data());
    }
}


auto editor_app::cli_setup(boost::program_options::options_description& options) -> void
{
    options.add_options()(editor_app_cli_arguments::s_projectFile, boost::program_options::value<std::string>(), "set the file path to the priject file");
}

auto editor_app::pre_init(const boost::program_options::variables_map& cliVariables) -> void
{
    const bool hasProjectFile = cliVariables.count(editor_app_cli_arguments::s_projectFile);

    if (hasProjectFile)
    {
        m_projectFile = cliVariables.at(editor_app_cli_arguments::s_projectFile).as<std::string>();
    }
}

void editor_app::post_init()
{
    std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    std::shared_ptr<root_widget> rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget.get());

    add_manager_class<app_manager>();

    std::shared_ptr<gluten::renderer_subsystem> renderedSubsystem = get_subsystem_by_class<gluten::renderer_subsystem>();
    renderedSubsystem->set_maximised();

    if (m_projectFile.has_value())
    {
        open_project(m_projectFile.value());
    }
}

auto editor_app::on_file_drop(const std::vector<std::string>& paths) -> void
{
    for (const std::string& path : paths)
    {
        std::filesystem::path filePath(path);

        if (std::filesystem::exists(filePath))
        {
            if (filePath.extension() == sbk::editor::project_configuration::projectExtensionWithDot)
            {
                open_project(filePath);
                return;
            }
            else if (filePath.extension() == ".wav")
            {
                if (sbk::editor::project* const project = sbk::engine::system::get()->get_project())
                {
                    const std::filesystem::path newFilePath = project->get_config().source_folder() / filePath.filename();

                    std::filesystem::copy_file(filePath, newFilePath);
                    auto createdSoundResult = project->create_database_object<sbk::engine::sound>();
                    if (createdSoundResult.has_value())
                    {
                        createdSoundResult.value()->set_object_name(newFilePath.filename().stem().string());
                        createdSoundResult.value()->set_sound_name(std::filesystem::relative(newFilePath, project->get_config().source_folder()).string());
                    }
                }
            }
        }
    }

    if (const sbk::editor::project* const project = sbk::engine::system::get()->get_project())
    {
        (void)project->save_project();
    }
}
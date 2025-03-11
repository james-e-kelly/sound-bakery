#include "app.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/sound/sound.h"
#include "widgets/root_widget.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

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

void editor_app::post_init()
{
    std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    std::shared_ptr<root_widget> rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget.get());

    add_manager_class<app_manager>();
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
                if (sbk::editor::project* const project = sbk::engine::system::get_project())
                {
                    const std::filesystem::path newFilePath = project->get_config().source_folder() / filePath.filename();

                    std::filesystem::copy_file(filePath, newFilePath);
                    if (std::shared_ptr<sbk::engine::sound> createdSound =
                            project->create_database_object<sbk::engine::sound>())
                    {
                        createdSound->set_database_name(newFilePath.filename().stem().string());
                        createdSound->set_sound_name(
                            std::filesystem::relative(newFilePath, project->get_config().source_folder()).string());
                    }
                }
            }
        }
    }

    if (const sbk::editor::project* const project = sbk::engine::system::get_project())
    {
        project->save_project();
    }
}
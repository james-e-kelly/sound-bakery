#include "project_manager.h"

#include "app/app.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "widgets/audio_meter_widget.h"
#include "widgets/details_widget.h"
#include "widgets/play_controls_widget.h"
#include "widgets/project_explorer_widget.h"
#include "widgets/profiler_widget.h"

void project_manager::init_project(const std::filesystem::path& project_file)
{
    if (sbk::engine::system::open_project(project_file) == MA_SUCCESS)
    {
        setup_project();
    }
}

auto project_manager::create_project(const std::filesystem::directory_entry& projectDirectory,
                                     const std::string& projectName) -> void
{
    if (sbk::engine::system::create_project(projectDirectory, projectName) == MA_SUCCESS)
    {
        setup_project();
    }
}

void project_manager::setup_project()
{
    m_projectExplorerWidget = get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<project_explorer_widget>(false);
    m_playerWidget = get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<player_widget>(false);
    m_detailsWidget = get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<details_widget>(false);
    m_audioMeterWidget = get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<audio_meter_widget>(false);
    m_profilerWidget = get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<profiler_widget>(false);

    m_projectExplorerWidget->set_visible_in_toolbar(true, false);
    m_playerWidget->set_visible_in_toolbar(true, false);
    m_detailsWidget->set_visible_in_toolbar(true, false);
    m_audioMeterWidget->set_visible_in_toolbar(true, false);
    m_profilerWidget->set_visible_in_toolbar(true, false);

    get_app()->set_application_display_title(
        fmt::format("{} - {} {}", sbk::engine::system::get_project()->get_config().project_name(), SBK_PRODUCT_NAME,
                    SBK_VERSION_STRING));
}

void project_manager::tick(double deltaTime)
{
    ZoneScoped;
    sbk::engine::system::update();
}

void project_manager::exit()
{
    ZoneScoped;
    sbk::engine::system::destroy();

    get_app()->set_application_display_title(SBK_PRODUCT_NAME " " SBK_VERSION_STRING);
}

void project_manager::save_project() const { sbk::engine::system::get()->get_project()->save_project(); }

sbk::engine::sound_container* project_manager::get_preview_sound_container() const
{
    return sbk::engine::system::get_project()->get_preview_container().lock().get();
}
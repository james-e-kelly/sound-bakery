#include "ProjectManager.h"

#include "App/App.h"
#include "Subsystems/WidgetSubsystem.h"
#include "Widgets/DetailsWidget.h"
#include "Widgets/PlayControlsWidget.h"
#include "Widgets/ProjectExplorerWidget.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

#include "sound_chef/sound_chef_encoder.h"

void ProjectManager::Init(const std::filesystem::path& projectFile)
{
    SB::Engine::System::create();
    SB::Engine::System::init();
    SB::Engine::System::openProject(projectFile);

    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<ProjectExplorerWidget>();
    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<PlayerWidget>();
    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<DetailsWidget>();

    m_previewSoundContainer =
        SB::Reflection::cast<SB::Engine::SoundContainer*, SB::Core::Object*>(
            SB::Engine::Factory::createObjectFromType(
                SB::Engine::SoundContainer::type()));
    m_previewSoundContainer->setDatabaseName("Preview Node");

    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        objectTracker->untrackObject(m_previewSoundContainer);
    }

    if (SB::Core::Database* const database = SB::Engine::System::getDatabase())
    {
        m_previewSoundContainerResource = database->removeUnsafe(m_previewSoundContainer);
    }
}

void ProjectManager::Tick(double deltaTime)
{
    SB::Engine::System::update();
}

void ProjectManager::Exit() 
{ 
    m_previewSoundContainerResource.reset();

    SB::Engine::System::destroy(); 
}

void ProjectManager::SaveProject() const 
{ 
    SB::Engine::System::get()->getProject()->saveProject(); 
}

void ProjectManager::ConvertAllFilesTest() const
{

    SB::Editor::Project* editorProject = SB::Engine::System::getProject();
    const SB::Editor::ProjectConfiguration& projectConfig = editorProject->getConfig();
    std::filesystem::path buildFolder = projectConfig.buildFolder();

    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        for (SB::Core::Object* const soundObject : objectTracker->getObjectsOfType(SB::Engine::Sound::type()))
        {
            if (SB::Engine::Sound* const sound = soundObject->tryConvertObject<SB::Engine::Sound>())
            {
                if (sc_sound* const lowSound = sound->getSound())
                {
                    if (ma_data_source* const dataSource = lowSound->sound.pDataSource)
                    {
                        std::filesystem::path soundFileName    = sound->getSoundName();
                        std::filesystem::path encodedSoundFile = buildFolder / soundFileName.filename();
                        encodedSoundFile.replace_extension("ogg");

                        std::filesystem::create_directories(encodedSoundFile.parent_path());

                        ma_uint32 channels = 0;

                        ma_data_source_get_data_format(dataSource, NULL, &channels, NULL, NULL, NULL);

                        sc_encoder_config encoderConfig = sc_encoder_config_init((ma_encoding_format_ext)ma_encoding_format_vorbis, ma_format_s24, channels, ma_standard_sample_rate_48000, 8);

                        sc_encoder_write_from_data_source(encodedSoundFile.string().c_str(), dataSource,
                                                          &encoderConfig);
                    }
                }
            }
        }
    }
}
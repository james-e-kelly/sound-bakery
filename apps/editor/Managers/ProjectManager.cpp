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
#include "sound_bakery/sound/sound.h"

#include "sound_bakery/editor/project/project.h"

using namespace SB::Engine;

DEFINE_REFLECTION(SB::Engine::Sound)

Sound::Sound() : m_streaming(false) {}

void Sound::loadSynchronous()
{
    std::filesystem::path finalSoundPath;

    if (!encodedSoundPath.empty())
    {
        if (std::filesystem::exists(encodedSoundPath))
        {
            finalSoundPath = encodedSoundPath;
            encodedSoundPath = std::filesystem::relative(encodedSoundPath,
                                                         SB::Engine::System::getProject()->getConfig().sourceFolder());
        }
        else
        {
            finalSoundPath = SB::Engine::System::getProject()->getConfig().encodedFolder() / encodedSoundPath;
        }
    }
    else if (!rawSoundPath.empty())
    {
        if (std::filesystem::exists(rawSoundPath))
        {
            finalSoundPath = rawSoundPath;
            rawSoundPath =
                std::filesystem::relative(rawSoundPath, SB::Engine::System::getProject()->getConfig().sourceFolder());
        }
        else
        {
            finalSoundPath = SB::Engine::System::getProject()->getConfig().sourceFolder() / rawSoundPath;
        }
    }

    assert(std::filesystem::exists(finalSoundPath));

    sc_sound* loadedSound = nullptr;

    sc_result result =
        sc_system_create_sound(getChef(), finalSoundPath.string().c_str(), SC_SOUND_MODE_DEFAULT, &loadedSound);
    assert(result == MA_SUCCESS);

    m_sound.reset(loadedSound);
}

void Sound::loadAsynchronous()
{
    loadSynchronous();
}

void Sound::setSoundName(std::string soundName)
{
    rawSoundPath = soundName;
    loadSynchronous();
}

std::string Sound::getSoundName() const 
{
    return rawSoundPath.string().c_str();
}
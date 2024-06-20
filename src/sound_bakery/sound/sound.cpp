#include "sound_bakery/sound/sound.h"

#include "sound_bakery/editor/project/project.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::Sound)

Sound::Sound() : m_streaming(false) {}

void Sound::loadSynchronous()
{
    std::filesystem::path finalSoundPath;

    bool useRawSound = true;

    // Prefer encoded media
    if (!encodedSoundPath.empty())
    {
        // Absolute path. Needs to be relative
        if (std::filesystem::exists(encodedSoundPath))
        {
            finalSoundPath   = encodedSoundPath;
            encodedSoundPath = std::filesystem::relative(
                encodedSoundPath, sbk::engine::system::get_project()->getConfig().encodedFolder());
        }
        else
        {
            finalSoundPath = sbk::engine::system::get_project()->getConfig().encodedFolder() / encodedSoundPath;
        }

        useRawSound = !std::filesystem::exists(finalSoundPath);
    }

    if (useRawSound && !rawSoundPath.empty())
    {
        if (std::filesystem::exists(rawSoundPath))
        {
            finalSoundPath = rawSoundPath;
            rawSoundPath =
                std::filesystem::relative(rawSoundPath, sbk::engine::system::get_project()->getConfig().sourceFolder());
        }
        else
        {
            finalSoundPath = sbk::engine::system::get_project()->getConfig().sourceFolder() / rawSoundPath;
        }
    }

    assert(std::filesystem::exists(finalSoundPath));

    sc_sound* loadedSound = nullptr;

    sc_result result =
        sc_system_create_sound(sbk::engine::system::get(), finalSoundPath.string().c_str(), SC_SOUND_MODE_DEFAULT, &loadedSound);
    assert(result == MA_SUCCESS);

    m_sound.reset(loadedSound);
}

void Sound::loadAsynchronous() { loadSynchronous(); }

void Sound::setSoundName(std::string soundName)
{
    rawSoundPath = soundName;
    loadSynchronous();
}

std::string Sound::getSoundName() const { return rawSoundPath.string().c_str(); }

void Sound::setEncodedSoundName(std::string encodedSoundName)
{
    encodedSoundPath = encodedSoundName;
    loadSynchronous();
}

sc_sound* Sound::getSound()
{
    if (!m_sound)
    {
        loadSynchronous();
    }

    return m_sound.get();
}
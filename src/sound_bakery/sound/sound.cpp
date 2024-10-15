#include "sound_bakery/sound/sound.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::sound)

sound::sound() : m_streaming(false) {}

void sound::loadSynchronous()
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
                encodedSoundPath, sbk::engine::system::get_project()->get_config().encoded_folder());
        }
        else
        {
            finalSoundPath = sbk::engine::system::get_project()->get_config().encoded_folder() / encodedSoundPath;
        }

        useRawSound = !std::filesystem::exists(finalSoundPath);
    }

    if (useRawSound && !rawSoundPath.empty())
    {
        if (std::filesystem::exists(rawSoundPath))
        {
            finalSoundPath = rawSoundPath;
            rawSoundPath   = std::filesystem::relative(rawSoundPath,
                                                       sbk::engine::system::get_project()->get_config().source_folder());
        }
        else
        {
            finalSoundPath = sbk::engine::system::get_project()->get_config().source_folder() / rawSoundPath;
        }
    }

    assert(std::filesystem::exists(finalSoundPath));

    sc_sound* loadedSound = nullptr;

    sc_result result = sc_system_create_sound(sbk::engine::system::get(), finalSoundPath.string().c_str(),
                                              SC_SOUND_MODE_DEFAULT, &loadedSound);
    assert(result == MA_SUCCESS);

    m_sound.reset(loadedSound);
}

void sound::loadAsynchronous() { loadSynchronous(); }

void sound::setSoundName(std::string soundName)
{
    rawSoundPath = soundName;
    loadSynchronous();
}

std::string sound::getSoundName() const { return rawSoundPath.string().c_str(); }

void sound::setEncodedSoundName(std::string encodedSoundName)
{
    encodedSoundPath = encodedSoundName;
    loadSynchronous();
}

sc_sound* sound::getSound()
{
    if (!m_sound)
    {
        loadSynchronous();
    }

    return m_sound.get();
}

encoding_sound sound::get_encoding_sound_data() const
{
    const sbk::editor::project_configuration projectConfig = sbk::engine::system::get_project()->get_config();

    encoding_sound encodingSound;

    encodingSound.rawSoundPath     = projectConfig.source_folder() / rawSoundPath;
    encodingSound.encodedSoundPath = projectConfig.encoded_folder() / encodedSoundPath;
    encodingSound.encodingFormat   = m_encodingFormat;

    return encodingSound;
}
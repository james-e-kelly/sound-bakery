#include "sound_bakery/sound/sound.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::sound)

void sound::load_synchronous()
{
    sc_sound* loadedSound = nullptr;

    switch (sbk::engine::system::get_operating_mode())
    {
        case sbk::engine::system::operating_mode::editor:
        {
            auto get_sound_file = [](const std::filesystem::path& file, const std::filesystem::path& containingFolder) -> std::filesystem::path 
            {
                if (!file.empty())
                {
                    if (std::filesystem::exists(file))
                    {
                        return file;
                    }
                    else
                    {
                        const std::filesystem::path convertedRelativeFile = containingFolder / file;

                        if (std::filesystem::exists(convertedRelativeFile))
                        {
                            return convertedRelativeFile;
                        }
                    }
                }
                return {};
            };

            std::filesystem::path finalSoundPath = get_sound_file(encodedSoundPath, sbk::engine::system::get_project()->get_config().encoded_folder());

            if (finalSoundPath.empty())
            {
                finalSoundPath = get_sound_file(rawSoundPath, sbk::engine::system::get_project()->get_config().source_folder());
            }
            BOOST_ASSERT(std::filesystem::exists(finalSoundPath));

            sc_result result = sc_system_create_sound(sbk::engine::system::get(), finalSoundPath.string().c_str(), SC_SOUND_MODE_DEFAULT, &loadedSound);
            BOOST_ASSERT(result == MA_SUCCESS);
            break;
        }
        case sbk::engine::system::operating_mode::runtime:
        {
            if (m_memorySoundData)
            {
                sc_result result =
                    sc_system_create_sound_memory(sbk::engine::system::get(), m_memorySoundData.get(),
                                                  m_memorySoundDataSize, SC_SOUND_MODE_DEFAULT, &loadedSound);
                BOOST_ASSERT(result == MA_SUCCESS);
            }
            break;
        }
        case sbk::engine::system::operating_mode::unkown:
            BOOST_ASSERT(false);
            break;
    }

    if (loadedSound)
    {
        m_sound.reset(loadedSound);
    }
}

void sound::load_asynchronous() { load_synchronous(); }

void sound::set_sound_name(std::string soundName)
{
    rawSoundPath = soundName;
    load_synchronous();
}

std::string sound::get_sound_name() const { return rawSoundPath.string().c_str(); }

void sound::set_encoded_sound_name(std::string encodedSoundName)
{
    encodedSoundPath = encodedSoundName;
    load_synchronous();
}

sc_sound* sound::get_sound()
{
    if (!m_sound)
    {
        load_synchronous();
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
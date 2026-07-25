#include "sound_bakery/sound/sound.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::sound)

namespace
{
    auto get_first_existing_file(std::initializer_list<std::filesystem::path> candidates) -> sbk::result<std::filesystem::path>
    {
        for (const std::filesystem::path& candidate : candidates)
        {
            if (!candidate.empty() && std::filesystem::is_regular_file(candidate) && std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }
        return sbk::make_error(SBK_ERR_INVALID_FILE);
    }

    auto create_sound_from_file(const std::filesystem::path& file) -> sbk::result<sc_sound*>
    {
        sc_sound* loadedSound = nullptr;
        SBK_TRY_C(sc_system_create_sound(sbk::engine::system::get(), file.string().c_str(), SC_SOUND_MODE_DEFAULT, &loadedSound));
        return loadedSound;
    }

    auto create_sound_from_memory(const sbk::engine::raw_sound_ptr& rawSound, std::size_t size) -> sbk::result<sc_sound*>
    {
        SBK_CHECK(rawSound, SBK_ERR_NULL);
        sc_sound* loadedSound = nullptr;
        SBK_TRY_C(sc_system_create_sound_memory(sbk::engine::system::get(), rawSound.get(), size, SC_SOUND_MODE_DEFAULT, &loadedSound));
        return loadedSound;
    }
}  // namespace

auto sound::load_synchronous() -> sbk::result<void>
{
    ZoneScoped;

    sc_sound* loadedSound = nullptr;

    const sbk::engine::system* const system = sbk::engine::system::get();
    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);

    if (const sbk::editor::project* const project = system->get_project())
    {
        const std::filesystem::path encodedFolder = project->get_config().encoded_folder();
        const std::filesystem::path sourceFolder  = project->get_config().source_folder();

        SBK_TRY(loadedSound, get_first_existing_file(
                                 {
                                     encodedSoundPath,
                                     encodedFolder / encodedSoundPath,
                                     rawSoundPath,
                                     sourceFolder / rawSoundPath,
                                 })
                                 .and_then(create_sound_from_file));
    }
    else
    {
        SBK_TRY(loadedSound, create_sound_from_memory(m_memorySoundData, m_memorySoundDataSize));
    }

    SBK_CHECK(loadedSound != nullptr, SBK_ERR_NULL);

    m_sound.reset(loadedSound);

    return sbk::ok();
}

auto sound::load_asynchronous() -> sbk::async_result<void>
{
    /// @todo Properly load this on another thread
    co_return load_synchronous();
}

auto sound::set_sound_name(std::string soundName) -> void
{
    rawSoundPath = soundName;
    (void)load_synchronous();
}

auto sound::get_sound_name() const -> std::string { return rawSoundPath.string().c_str(); }

auto sound::set_encoded_sound_name(std::string encodedSoundName) -> void
{
    encodedSoundPath = encodedSoundName;
    (void)load_synchronous();
}

auto sound::get_sound() -> sc_sound*
{
    if (!m_sound)
    {
        (void)load_synchronous();
    }

    return m_sound.get();
}

auto sound::get_encoding_sound_data() const -> encoding_sound
{
    encoding_sound encodingSound;

    if (const sbk::engine::system* const system = sbk::engine::system::get())
    {
        if (const sbk::editor::project* const project = system->get_project())
        {
            const sbk::editor::project_configuration projectConfig = project->get_config();

            encodingSound.rawSoundPath     = projectConfig.source_folder() / rawSoundPath;
            encodingSound.encodedSoundPath = projectConfig.encoded_folder() / encodedSoundPath;
            encodingSound.encodingFormat   = m_encodingFormat;
        }
    }

    return encodingSound;
}
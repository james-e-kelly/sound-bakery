#include "audio_subsystem.h"

#include "gluten/app/app.h"

#define CHECK_SC_RESULT(result)     \
    if (result != SBK_SUCCESS)      \
    {                               \
        return 1;                   \
    }

auto gluten::audio_subsystem::sc_system_deleter::operator()(sc_system* system) -> void
{
    if (system)
    {
        sc_system_close(system);
        sc_system_release(system);
    }
}

int gluten::audio_subsystem::init()
{
    sc_system* system = nullptr;
    sbk_result result = sc_system_create(&system);
    CHECK_SC_RESULT(result);

    m_soundChef.reset(system);

    if (m_soundChef)
    {
        const sc_system_config config = sc_system_config_init_default();
        result = sc_system_init(m_soundChef.get(), &config);
        CHECK_SC_RESULT(result);
    }

    return 0;
}

auto gluten::audio_subsystem::play_sound(const std::filesystem::path& filePath) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_bool playing = MA_FALSE;
        sc_sound_instance_is_playing(soundInstance, &playing);

        if (!playing)
        {
            sc_sound_instance_start(soundInstance);
        }
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_instance* soundInstance = nullptr;
        sc_system_play_sound(m_soundChef.get(), sound, &soundInstance, nullptr, SBK_FALSE);
        m_filesToSoundInstancesMap[filePath].reset(soundInstance);
    }
}

auto gluten::audio_subsystem::pause_sound(const std::filesystem::path& filePath) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_bool playing = MA_FALSE;
        sc_sound_instance_is_playing(soundInstance, &playing);

        if (playing)
        {
            sc_sound_instance_pause(soundInstance);
        }
    }
}

auto gluten::audio_subsystem::set_sound_cursor_position(const std::filesystem::path& filePath, float cursorPosition) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_set_cursor_in_seconds(soundInstance, cursorPosition);
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_instance* soundInstance = nullptr;
        sc_system_play_sound(m_soundChef.get(), sound, &soundInstance, nullptr, SBK_TRUE);
        sc_sound_instance_set_cursor_in_seconds(soundInstance, cursorPosition);
        m_filesToSoundInstancesMap[filePath].reset(soundInstance);
    }
}

auto gluten::audio_subsystem::get_sound_cursor_position(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_get_cursor_in_seconds(soundInstance, &seconds);
    }

    return seconds;
}

auto gluten::audio_subsystem::get_sound_length(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

   if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_get_length(sound, &seconds);
    }

    return seconds;
}

auto gluten::audio_subsystem::get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*
{
    sc_sound* sound = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundsMap.contains(filePath))
        {
            sound = m_filesToSoundsMap.at(filePath).get();
        }
        else if (sc_system_create_sound(m_soundChef.get(), filePath.string().c_str(), SC_SOUND_MODE_DECODE, &sound) == SBK_SUCCESS)
        {
            m_filesToSoundsMap[filePath].reset(sound);
        }
    }
    return sound;
}

auto gluten::audio_subsystem::get_sound_instance(const std::filesystem::path& filePath) -> sc_sound_instance*
{
    sc_sound_instance* soundInstance = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundInstancesMap.contains(filePath))
        {
            soundInstance = m_filesToSoundInstancesMap.at(filePath).get();
        }
    }
    return soundInstance;
}

auto gluten::audio_subsystem::get_ui_waveform(const std::filesystem::path& filePath, std::size_t buckets) -> waveform&
{
    if (!m_filesToWaveforms.contains(filePath))
    {
        m_filesToWaveforms.insert({filePath, waveform()});
        async_generate_waveform(filePath, buckets);
    }
    return m_filesToWaveforms.at(filePath);
}

auto gluten::audio_subsystem::async_generate_waveform(const std::filesystem::path filePath, std::size_t targetSamples) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    constexpr std::size_t bucketsToFillPerIteration = 50;
    std::size_t iteration                           = 0;

    std::vector<waveform_bucket> buckets;

    for (const waveform_bucket bucket : generate_waveform(filePath, targetSamples))
    {
        buckets.push_back(bucket);

        if (iteration++ >= bucketsToFillPerIteration)
        {
            iteration = 0;
            co_await concurrencpp::resume_on(gluten::app::get()->get_tick_executor());
            m_filesToWaveforms.at(filePath).insert(m_filesToWaveforms.at(filePath).end(), buckets.begin(), buckets.end());
            co_await concurrencpp::resume_on(gluten::app::get()->background_executor());
            buckets.clear();
        }
    }

    co_return;
}

auto gluten::audio_subsystem::generate_waveform(const std::filesystem::path filePath, std::size_t targetSamples) -> waveform_generator
{
    if (!std::filesystem::exists(filePath))
    {
        co_yield std::vector<std::pair<float, float>>();
    }

    if (targetSamples == 0)
    {
        co_yield std::vector<std::pair<float, float>>();
    }

    ma_decoder decoder;
    SC_ZERO_OBJECT(&decoder);
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, ma_standard_sample_rate_48000);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) != MA_SUCCESS)
    {
        co_yield std::vector<std::pair<float, float>>();
    }

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    size_t framesPerBucket = frameCount / targetSamples;
    if (framesPerBucket < 1)
    {
        framesPerBucket = 1;
    }

    std::vector<std::pair<float, float>> result;
    result.reserve(decoder.outputChannels);

    std::vector<float> framesInBucket(framesPerBucket * decoder.outputChannels, 0.0f);
    std::vector<float> channelMaxesForBucket(decoder.outputChannels, 0.0f);
    std::vector<float> channelMinsForBucket(decoder.outputChannels, 0.0f);

    for (ma_uint64 bucket = 0; bucket < frameCount; bucket += framesPerBucket)
    {
        std::fill(framesInBucket.begin(), framesInBucket.end(), 0.0f);
        std::fill(channelMaxesForBucket.begin(), channelMaxesForBucket.end(), 0.0f);
        std::fill(channelMinsForBucket.begin(), channelMinsForBucket.end(), 0.0f);

        ma_decoder_read_pcm_frames(&decoder, framesInBucket.data(), framesPerBucket, NULL);

        for (ma_uint64 frame = 0; frame < framesPerBucket; ++frame)
        {
            for (ma_uint64 channel = 0; channel < decoder.outputChannels; ++channel)
            {
                const ma_uint64 sampleIndex = (frame * decoder.outputChannels) + channel;
                const float sampleValue = framesInBucket[sampleIndex];
                if (sampleValue > channelMaxesForBucket[channel])
                {
                    channelMaxesForBucket[channel] = sampleValue;
                }

                if (sampleValue < channelMinsForBucket[channel])
                {
                    channelMinsForBucket[channel] = sampleValue;
                }
            }
        }

        result.clear();

        for (ma_uint64 channel = 0; channel < decoder.outputChannels; ++channel)
        {
            result.push_back(std::pair<float, float>(channelMinsForBucket[channel], channelMaxesForBucket[channel]));
        }

        co_yield result;
    }

    ma_decoder_uninit(&decoder);
}
#include "audio_subsystem.h"

#include "sound_bakery/runtime/runtime.h"

#include "ebur128/ebur128.h"
#include "gluten/app/app.h"

#define CHECK_SC_RESULT(result) \
    if (result != SBK_SUCCESS)  \
    {                           \
        return 1;               \
    }

static void sbk_log_callback(unsigned int level, const char* message)
{
    const sbk::core::sbk_log_level logLevel = static_cast<sbk::core::sbk_log_level>(level);

    switch (logLevel)
    {
        case sbk::core::SBK_LOG_LEVEL_DEBUG:
            gluten::app::get()->get_logger()->log(spdlog::level::debug, message);
            break;
        case sbk::core::SBK_LOG_LEVEL_INFO:
            gluten::app::get()->get_logger()->log(spdlog::level::info, message);
            break;
        case sbk::core::SBK_LOG_LEVEL_WARNING:
            gluten::app::get()->get_logger()->log(spdlog::level::warn, message);
            break;
        case sbk::core::SBK_LOG_LEVEL_ERROR:
            gluten::app::get()->get_logger()->log(spdlog::level::err, message);
            break;
    }
}

int gluten::audio_subsystem::init()
{
    sbk_status result = sbk_system_create();
    if (result != SBK_SUCCESS)
    {
        return 1;
    }

    result = sbk_system_init(sbk_system_config_init_default());
    if (result != SBK_SUCCESS)
    {
        return 1;
    }    

    return 0;
}

auto gluten::audio_subsystem::play_sound(const std::filesystem::path& filePath) -> void
{
    loop_data* loopData = get_sound_loop_info(filePath);

    const auto try_set_loop_points = [](sc_sound_instance* soundInstance, loop_data* loopData)
    {
        if (soundInstance && loopData)
        {
            if (loopData->m_loopStart >= 0.0f && loopData->m_loopEnd >= 0.0f)
            {
                sc_sound_instance_set_loop_position_in_seconds(soundInstance, loopData->m_loopStart, loopData->m_loopEnd);
            }
        }
    };

    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_bool playing = MA_FALSE;
        sc_sound_instance_is_playing(soundInstance, &playing);

        if (!playing)
        {
            sc_sound_instance_start(soundInstance);
            try_set_loop_points(soundInstance, loopData);
        }
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_instance* soundInstance = nullptr;
        sc_system_play_sound(sbk::engine::system::get()->get_runtime(), sound, &soundInstance, nullptr, SBK_FALSE);
        try_set_loop_points(soundInstance, loopData);
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

auto gluten::audio_subsystem::pause_all() -> void
{
    for (const auto& soundInstance : m_filesToSoundInstancesMap)
    {
        if (soundInstance.second)
        {
            sc_sound_instance_pause(soundInstance.second.get());
        }
    }
}

auto gluten::audio_subsystem::set_sound_cursor_position(const std::filesystem::path& filePath, float cursorPosition) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_set_cursor_in_seconds(soundInstance, cursorPosition);
        sc_sound_instance_set_looping(soundInstance, SBK_FALSE);
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_instance* soundInstance = nullptr;
        sc_system_play_sound(sbk::engine::system::get()->get_runtime(), sound, &soundInstance, nullptr, SBK_TRUE);
        sc_sound_instance_set_cursor_in_seconds(soundInstance, cursorPosition);
        sc_sound_instance_set_looping(soundInstance, SBK_FALSE);
        m_filesToSoundInstancesMap[filePath].reset(soundInstance);
    }
}

auto gluten::audio_subsystem::set_sound_loop_start_position(const std::filesystem::path& filePath, float loopPosition) -> void
{
    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
        {
            sc_sound_instance_set_looping(soundInstance, SBK_FALSE);
        }

        loopData->m_loopStart = loopPosition;
    }
}

auto gluten::audio_subsystem::set_sound_loop_end_position(const std::filesystem::path& filePath, float loopPosition) -> void
{
    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
        {
            sc_sound_instance_set_looping(soundInstance, SBK_TRUE);
        }

        loopData->m_loopEnd = loopPosition;
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

auto gluten::audio_subsystem::get_sound_loop_start_position(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        seconds = loopData->m_loopStart;
    }

    return seconds;
}

auto gluten::audio_subsystem::get_sound_loop_end_position(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        seconds = loopData->m_loopEnd;
    }

    return seconds;
}

auto gluten::audio_subsystem::get_sound_length(const std::filesystem::path& filePath) -> float
{
    ZoneScoped;

    float seconds = 0.0f;

    if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_get_length(sound, &seconds);
    }

    return seconds;
}

auto gluten::audio_subsystem::get_sound_is_playing(const std::filesystem::path& filePath) -> bool
{
    sc_bool playing = MA_FALSE;

    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_is_playing(soundInstance, &playing);
    }

    return playing;
}

auto gluten::audio_subsystem::get_sound_is_looping(const std::filesystem::path& filePath) -> bool
{
    sc_bool looping = MA_FALSE;

    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_get_is_looping(soundInstance, &looping);
    }

    return looping;
}

auto gluten::audio_subsystem::stop_all_sounds() -> void
{
    for (const auto& soundInstance : m_filesToSoundInstancesMap)
    {
        if (soundInstance.second)
        {
            sc_sound_instance_pause(soundInstance.second.get());
        }
    }

    m_filesToSoundInstancesMap.clear();
}

auto gluten::audio_subsystem::get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*
{
    ZoneScoped;

    sc_sound* sound = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundsMap.contains(filePath))
        {
            sound = m_filesToSoundsMap.at(filePath).get();
        }
        else if (sc_system_create_sound(sbk::engine::system::get()->get_runtime(), filePath.string().c_str(), SC_SOUND_MODE_DECODE, &sound) == SBK_SUCCESS)
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

auto gluten::audio_subsystem::get_ui_waveform_lods(const std::filesystem::path& filePath, double fileDuration) -> waveform_lods_cache_type::cache_result
{
    ZoneScoped;

    if (m_waveformLodCache.get_cache_needs_filling(filePath))
    {
        m_waveformLodCache.set_async_fill_cache(filePath, async_generate_waveform_lods(filePath, fileDuration, 0));
    }

    return m_waveformLodCache.get_cached_data(filePath);
}

auto gluten::audio_subsystem::get_sound_loop_info(const std::filesystem::path& filePath) -> loop_data*
{
    loop_data* loopData = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (!m_filesToLoopDataMap.contains(filePath))
        {
            m_filesToLoopDataMap.insert({filePath, loop_data()});
        }

        loopData = &m_filesToLoopDataMap.at(filePath);
    }
    return loopData;
}

struct fiber_raii
{
    ~fiber_raii()
    {
        TracyFiberLeave;
    }
};

#define SET_UP_FIBER                                                                                                    \
    static std::atomic<unsigned long> counter;                                                                          \
    const std::string _fiberName = fmt::format("{}_{}", __FUNCTION__, counter.fetch_add(1, std::memory_order_relaxed)); \
    const fiber_raii fiberRaii

#define ENTER_FIBER TracyFiberEnter(_fiberName.c_str())
#define EXIT_FIBER  TracyFiberLeave

auto gluten::audio_subsystem::async_generate_waveform_lod(std::shared_ptr<const std::vector<float>> audioData, ma_uint64 channels, double fileDuration, std::size_t resolution, concurrencpp::shared_result<waveform_lod> dependencyResult) -> concurrencpp::result<waveform_lod>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    SET_UP_FIBER;

    ENTER_FIBER;

    if (dependencyResult)
    {
        EXIT_FIBER;
        co_await dependencyResult;
        ENTER_FIBER;
    }

    waveform_lod lod;
    lod.resolution = resolution;

    if (resolution == ma_standard_sample_rate_48000)
    {
        EXIT_FIBER;
        lod.lodWaveform = co_await generate_sample_resolution_waveform(audioData, channels);
        ENTER_FIBER;
    }
    else
    {
        EXIT_FIBER;
        lod.lodWaveform = co_await generate_downsampled_resolution_waveform(audioData, resolution, channels, resolution * fileDuration);
        ENTER_FIBER;
    }

    co_return lod;
}

auto gluten::audio_subsystem::async_generate_waveform_lods(const std::filesystem::path filePath, double fileDuration, std::size_t resolution) -> concurrencpp::result<waveform_lods>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    waveform_lods result;

    ma_decoder decoder;
    SC_ZERO_OBJECT(&decoder);

    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, ma_standard_sample_rate_48000);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) == MA_SUCCESS)
    {
        ma_uint64 frameCount;
        ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

        std::shared_ptr<const std::vector<float>> frameData = std::make_shared<const std::vector<float>>(frameCount * decoder.outputChannels, 0.0f);
        ma_decoder_read_pcm_frames(&decoder, const_cast<float*>(frameData->data()), frameCount, nullptr);

        concurrencpp::shared_result<waveform_lod> thumbnailResult = async_generate_waveform_lod(frameData, decoder.outputChannels, fileDuration, 10, {});

        result.thumbnailRes.set_async_fill_cache(thumbnailResult);
        result.lowRes.set_async_fill_cache(async_generate_waveform_lod(frameData, decoder.outputChannels, fileDuration, 100, thumbnailResult));
        result.medRes.set_async_fill_cache(async_generate_waveform_lod(frameData, decoder.outputChannels, fileDuration, 1000, thumbnailResult));
        result.highRes.set_async_fill_cache(async_generate_waveform_lod(frameData, decoder.outputChannels, fileDuration, 3000, thumbnailResult));
        result.sampleRes.set_async_fill_cache(async_generate_waveform_lod(frameData, decoder.outputChannels, fileDuration, 48000, thumbnailResult));
    }

    co_return result;
}

auto gluten::audio_subsystem::generate_downsampled_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<waveform>
{
    SET_UP_FIBER;

    ENTER_FIBER;

    ZoneScoped;

    if (!audioData || audioData->empty())
    {
        co_return waveform();
    }

    if (targetSamples == 0)
    {
        co_return waveform();
    }

    const std::size_t frameCount = audioData->size() / channels;
    size_t framesToRead          = frameCount / targetSamples;
    if (framesToRead < 1)
    {
        framesToRead = 1;
    }

    waveform result(targetSamples, channels);

    if (resolution <= 1000)
    {
        EXIT_FIBER;
        result.globalFramesCache.set_async_fill_cache(generate_downsampled_resolution_global_frames(audioData, resolution, channels, targetSamples));
        ENTER_FIBER;
    }

    const float channelsReciprocal     = 1.0f / channels;
    const float framesToReadReciprocal = 1.0f / framesToRead;

    for (ma_uint64 samplingIndex = 0; samplingIndex < targetSamples; ++samplingIndex)
    {
        for (ma_uint64 frame = 0; frame < framesToRead; ++frame)
        {
            float allChannelsSum = 0.0f;

            for (ma_uint64 channel = 0; channel < channels; ++channel)
            {
                const ma_uint64 sampleIndex = (samplingIndex * framesToRead * channels) + (frame * channels) + channel;

                const float& sampleValue = audioData->at(sampleIndex);

                allChannelsSum += std::abs(sampleValue) * channelsReciprocal;

                result.channelFrames[channel][samplingIndex].min = std::min<float>(result.channelFrames[channel][samplingIndex].min, sampleValue);
                result.channelFrames[channel][samplingIndex].max = std::max<float>(result.channelFrames[channel][samplingIndex].max, sampleValue);
            }

            if (channels == 2)
            {
                const ma_uint64 leftSampleIndex  = (samplingIndex * framesToRead * channels) + (frame * channels) + 0;
                const ma_uint64 rightSampleIndex = (samplingIndex * framesToRead * channels) + (frame * channels) + 1;

                const float leftSampleValue  = audioData->at(leftSampleIndex);
                const float rightSampleValue = audioData->at(rightSampleIndex);

                const float midValue  = (leftSampleValue + rightSampleValue) * channelsReciprocal;
                const float sideValue = (leftSampleValue - rightSampleValue) * channelsReciprocal;

                result.stereoFrames[samplingIndex].midMin  = std::min<float>(result.stereoFrames[samplingIndex].midMin, midValue);
                result.stereoFrames[samplingIndex].midMax  = std::max<float>(result.stereoFrames[samplingIndex].midMax, midValue);
                result.stereoFrames[samplingIndex].sideMin = std::min<float>(result.stereoFrames[samplingIndex].sideMin, sideValue);
                result.stereoFrames[samplingIndex].sideMax = std::max<float>(result.stereoFrames[samplingIndex].sideMax, sideValue);
            }
        }
    }

    co_return result;
}

auto gluten::audio_subsystem::generate_downsampled_resolution_global_frames(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<std::vector<frame_data>>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    SET_UP_FIBER;

    ENTER_FIBER;

    ZoneScoped;

    constexpr double lowsCrossoverFrequency  = 250.0;
    constexpr double highsCrossoverFrequency = 4000.0;

    if (!audioData || audioData->empty())
    {
        co_return std::vector<frame_data>();
    }

    if (targetSamples == 0)
    {
        co_return std::vector<frame_data>();
    }

    ma_lpf lowsLowpass;
    ma_hpf highsHighpass;
    ma_lpf midsLowpass;
    ma_hpf midsHighpass;
    SC_ZERO_OBJECT(&lowsLowpass);
    SC_ZERO_OBJECT(&highsHighpass);
    SC_ZERO_OBJECT(&midsLowpass);
    SC_ZERO_OBJECT(&midsHighpass);

    ebur128_state* eburState = ebur128_init(channels, ma_standard_sample_rate_48000, EBUR128_MODE_M | EBUR128_MODE_S);

    const std::size_t frameCount = audioData->size() / channels;
    size_t framesToRead          = frameCount / targetSamples;
    if (framesToRead < 1)
    {
        framesToRead = 1;
    }

    const ma_lpf_config lpfConfig     = ma_lpf_config_init(ma_format_f32, channels, ma_standard_sample_rate_48000, lowsCrossoverFrequency, 2);
    const ma_hpf_config hpfConfig     = ma_hpf_config_init(ma_format_f32, channels, ma_standard_sample_rate_48000, highsCrossoverFrequency, 2);
    const ma_lpf_config midsLpfConfig = ma_lpf_config_init(ma_format_f32, channels, ma_standard_sample_rate_48000, highsCrossoverFrequency, 2);
    const ma_hpf_config midsHpfConfig = ma_hpf_config_init(ma_format_f32, channels, ma_standard_sample_rate_48000, lowsCrossoverFrequency, 2);

    ma_lpf_init(&lpfConfig, NULL, &lowsLowpass);
    ma_hpf_init(&hpfConfig, NULL, &highsHighpass);
    ma_lpf_init(&midsLpfConfig, NULL, &midsLowpass);
    ma_hpf_init(&midsHpfConfig, NULL, &midsHighpass);

    std::vector<frame_data> result(targetSamples, frame_data());

    std::vector<float> lowData(audioData->size(), 0.0f);
    std::vector<float> midData(audioData->size(), 0.0f);
    std::vector<float> highData(audioData->size(), 0.0f);

    {
        ZoneScopedN("Process Low And High Passes");

        ma_lpf_process_pcm_frames(&lowsLowpass, lowData.data(), audioData->data(), frameCount);
        ma_hpf_process_pcm_frames(&highsHighpass, highData.data(), audioData->data(), frameCount);

        ma_lpf_process_pcm_frames(&midsLowpass, midData.data(), audioData->data(), frameCount);
        ma_hpf_process_pcm_frames(&midsHighpass, midData.data(), midData.data(), frameCount);
    }

    const float channelsReciprocal     = 1.0f / channels;
    const float framesToReadReciprocal = 1.0f / framesToRead;

    const bool calculateLufs = resolution <= 10;  // Keep a very low resolution for LUFS. It doesn't change very fast

    for (ma_uint64 samplingIndex = 0; samplingIndex < targetSamples; ++samplingIndex)
    {
        ZoneScopedN("loop");

        if (calculateLufs)
        {
            ZoneScopedN("calculate_lufs");

            {
                ZoneScopedN("add_frames");
                ebur128_add_frames_float(eburState, audioData->data() + (samplingIndex * framesToRead * channels), framesToRead);
            }

            {
                ZoneScopedN("shortterm");
                ebur128_loudness_shortterm(eburState, &result[samplingIndex].lufs.shortterm);
            }

            {
                ZoneScopedN("momentary");
                ebur128_loudness_momentary(eburState, &result[samplingIndex].lufs.momentary);
            }
        }

        for (ma_uint64 frame = 0; frame < framesToRead; ++frame)
        {
            float allChannelsSum = 0.0f;

            for (ma_uint64 channel = 0; channel < channels; ++channel)
            {
                const ma_uint64 sampleIndex = (samplingIndex * framesToRead * channels) + (frame * channels) + channel;

                const float sampleValue = audioData->at(sampleIndex);
                const float lowValue    = lowData[sampleIndex];
                const float midValue    = midData[sampleIndex];
                const float highValue   = highData[sampleIndex];

                allChannelsSum += std::abs(sampleValue) * channelsReciprocal;

                result[samplingIndex].channelSumAverage += (std::abs(sampleValue) * channelsReciprocal) * framesToReadReciprocal;
                result[samplingIndex].lowAverage += (std::abs(lowValue) * channelsReciprocal) * framesToReadReciprocal;
                result[samplingIndex].midAverage += (std::abs(midValue) * channelsReciprocal) * framesToReadReciprocal;
                result[samplingIndex].highAverage += (std::abs(highValue) * channelsReciprocal) * framesToReadReciprocal;
            }

            result[samplingIndex].rms += allChannelsSum * allChannelsSum;
        }

        result[samplingIndex].rms *= framesToReadReciprocal;
    }

    ma_lpf_uninit(&lowsLowpass, NULL);
    ma_hpf_uninit(&highsHighpass, NULL);
    ma_lpf_uninit(&midsLowpass, NULL);
    ma_hpf_uninit(&midsHighpass, NULL);

    ebur128_destroy(&eburState);

    co_return result;
}

auto gluten::audio_subsystem::generate_sample_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, ma_uint32 channels) -> concurrencpp::result<waveform>
{
    SET_UP_FIBER;

    ENTER_FIBER;

    ZoneScoped;

    if (!audioData || audioData->empty())
    {
        co_return waveform();
    }

    const ma_uint64 frameCount = audioData->size() / channels;

    waveform result(frameCount, channels);

    for (ma_uint64 frame = 0; frame < frameCount; ++frame)
    {
        for (ma_uint64 channel = 0; channel < channels; ++channel)
        {
            const ma_uint64 sampleIndex                 = (frame * channels) + channel;
            result.channelFrames[channel][frame].sample = audioData->at(sampleIndex);
        }
    }

    {
        ZoneScopedN("Calculate LUFS");

        ebur128_state* eburState = ebur128_init(channels, ma_standard_sample_rate_48000, EBUR128_MODE_I | EBUR128_MODE_LRA | EBUR128_MODE_HISTOGRAM);

        ebur128_add_frames_float(eburState, audioData->data(), frameCount);
        ebur128_loudness_global(eburState, &result.lufs.integrated);
        ebur128_loudness_range(eburState, &result.lufs.range);

        ebur128_destroy(&eburState);
    }

    co_return result;
}
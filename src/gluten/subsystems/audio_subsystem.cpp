#include "audio_subsystem.h"

#include "ebur128/ebur128.h"
#include "gluten/app/app.h"

#define CHECK_SC_RESULT(result)     \
    if (result != SBK_SUCCESS)      \
    {                               \
        return 1;                   \
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
    m_soundBakery = std::make_unique<sbk::engine::system>(sbk_log_callback);

    if (m_soundBakery)
    {
        CHECK_SC_RESULT(m_soundBakery->init(sbk_system_config_init_default()));
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
        sc_system_play_sound(m_soundBakery.get(), sound, &soundInstance, nullptr, SBK_FALSE);
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
        sc_system_play_sound(m_soundBakery.get(), sound, &soundInstance, nullptr, SBK_TRUE);
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
    sc_sound* sound = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundsMap.contains(filePath))
        {
            sound = m_filesToSoundsMap.at(filePath).get();
        }
        else if (sc_system_create_sound(m_soundBakery.get(), filePath.string().c_str(), SC_SOUND_MODE_DECODE, &sound) == SBK_SUCCESS)
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

auto gluten::audio_subsystem::get_ui_waveform_lods(const std::filesystem::path& filePath, double fileDuration) -> waveform_lods&
{
    if (!m_filesToWaveformLods.contains(filePath))
    {
        waveform_lods lods;

        lods.thumbnailRes.set_async_fill_cache(filePath, async_generate_waveform_lod(filePath, fileDuration, 10));
        lods.lowRes.set_async_fill_cache(filePath, async_generate_waveform_lod(filePath, fileDuration, 100));
        lods.medRes.set_async_fill_cache(filePath, async_generate_waveform_lod(filePath, fileDuration, 1000));
        lods.highRes.set_async_fill_cache(filePath, async_generate_waveform_lod(filePath, fileDuration, 3000));
        lods.sampleRes.set_async_fill_cache(filePath, async_generate_waveform_lod(filePath, fileDuration, 48000));

        m_filesToWaveformLods.insert({filePath, std::move(lods)});
    }

    return m_filesToWaveformLods.at(filePath);
}

auto gluten::audio_subsystem::get_loudness_lufs(const std::filesystem::path& filePath) -> loudness_cache_type::cache_result
{
    if (m_filesToLoudnessCache.get_cache_needs_filling(filePath))
    {
        m_filesToLoudnessCache.set_async_fill_cache(filePath, async_calculate_loudness(filePath));
    }

    return m_filesToLoudnessCache.get_cached_data(filePath);
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

auto gluten::audio_subsystem::async_generate_waveform_lod(const std::filesystem::path filePath, double fileDuration, std::size_t resolution) -> concurrencpp::result<waveform_lod>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    waveform_lod lod;
    lod.resolution = resolution;

    if (resolution == ma_standard_sample_rate_48000)
    {
        lod.waveform = co_await generate_sample_resolution_waveform(filePath);
    }
    else
    {
        lod.waveform = co_await generate_downsampled_resolution_waveform(filePath, resolution * fileDuration);
    }

    co_return lod;
}

auto gluten::audio_subsystem::async_calculate_loudness(const std::filesystem::path filePath) -> loudness_cache_type::async_cache_result
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    ma_decoder decoder;
    SC_ZERO_OBJECT(&decoder);
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) != MA_SUCCESS)
    {
        co_return loudness_lufs();
    }

    loudness_lufs loudness;

    ebur128_state* eburState = ebur128_init(decoder.outputChannels, decoder.outputSampleRate, EBUR128_MODE_M | EBUR128_MODE_S | EBUR128_MODE_I);

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    const std::size_t framesPerLoop = decoder.outputSampleRate;
    std::vector<float> pcmData(decoder.outputChannels * framesPerLoop, 0.0f);

    while (frameCount)
    {
        const std::size_t framesToRead = std::min<std::size_t>(frameCount, framesPerLoop);
        frameCount -= framesToRead;

        ma_decoder_read_pcm_frames(&decoder, pcmData.data(), framesToRead, nullptr);
        ebur128_add_frames_float(eburState, pcmData.data(), framesToRead);

        double shortterm = 0.0;
        double momentary = 0.0;

        ebur128_loudness_shortterm(eburState, &shortterm);
        ebur128_loudness_momentary(eburState, &momentary);

        loudness.shorttermMax = std::max<double>(loudness.shorttermMax, shortterm);
        loudness.momentaryMax = std::max<double>(loudness.momentaryMax, momentary);

    }
    
    ebur128_loudness_global(eburState, &loudness.integrated);

    ebur128_destroy(&eburState);

    co_return loudness;
}

auto gluten::audio_subsystem::generate_downsampled_resolution_waveform(const std::filesystem::path filePath, std::size_t targetSamples) -> concurrencpp::result<waveform>
{
    constexpr double lowsCrossoverFrequency = 250.0;
    constexpr double highsCrossoverFrequency = 4000.0;

    if (!std::filesystem::exists(filePath))
    {
        co_return waveform();
    }

    if (targetSamples == 0)
    {
        co_return waveform();
    }

    ma_decoder decoder;
    ma_lpf lowsLowpass;
    ma_hpf highsHighpass;
    ma_lpf midsLowpass;
    ma_hpf midsHighpass;
    SC_ZERO_OBJECT(&decoder);
    SC_ZERO_OBJECT(&lowsLowpass);
    SC_ZERO_OBJECT(&highsHighpass);
    SC_ZERO_OBJECT(&midsLowpass);
    SC_ZERO_OBJECT(&midsHighpass);

    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, ma_standard_sample_rate_48000);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) != MA_SUCCESS)
    {
        co_return waveform();
    }

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    
    size_t framesPerBucket = frameCount / targetSamples;
    if (framesPerBucket < 1)
    {
        framesPerBucket = 1;
    }

    const ma_lpf_config lpfConfig       = ma_lpf_config_init(ma_format_f32, decoder.outputChannels, ma_standard_sample_rate_48000, lowsCrossoverFrequency, 2);
    const ma_hpf_config hpfConfig       = ma_hpf_config_init(ma_format_f32, decoder.outputChannels, ma_standard_sample_rate_48000, highsCrossoverFrequency, 2);
    const ma_lpf_config midsLpfConfig   = ma_lpf_config_init(ma_format_f32, decoder.outputChannels, ma_standard_sample_rate_48000, highsCrossoverFrequency, 2);
    const ma_hpf_config midsHpfConfig   = ma_hpf_config_init(ma_format_f32, decoder.outputChannels, ma_standard_sample_rate_48000, lowsCrossoverFrequency, 2);

    ma_lpf_init(&lpfConfig, NULL, &lowsLowpass);
    ma_hpf_init(&hpfConfig, NULL, &highsHighpass);
    ma_lpf_init(&midsLpfConfig, NULL, &midsLowpass);
    ma_hpf_init(&midsHpfConfig, NULL, &midsHighpass);

    waveform result(targetSamples, decoder.outputChannels);

    std::vector<float> frameData(frameCount * decoder.outputChannels, 0.0f);
    std::vector<float> lowData(framesPerBucket * decoder.outputChannels, 0.0f);
    std::vector<float> midData(framesPerBucket * decoder.outputChannels, 0.0f);
    std::vector<float> highData(framesPerBucket * decoder.outputChannels, 0.0f);

    ma_decoder_read_pcm_frames(&decoder, frameData.data(), frameCount, nullptr);

    for (ma_uint64 samplingIndex = 0; samplingIndex < targetSamples; ++samplingIndex)
    {
        ma_uint64 framesToRead    = framesPerBucket;

        ma_lpf_process_pcm_frames(&lowsLowpass, lowData.data(), frameData.data() + (samplingIndex * framesPerBucket * decoder.outputChannels), framesToRead);
        ma_hpf_process_pcm_frames(&highsHighpass, highData.data(), frameData.data() + (samplingIndex * framesPerBucket * decoder.outputChannels), framesToRead);

        ma_lpf_process_pcm_frames(&midsLowpass, midData.data(), frameData.data() + (samplingIndex * framesPerBucket * decoder.outputChannels), framesToRead);
        ma_hpf_process_pcm_frames(&midsHighpass, midData.data(), midData.data(), framesToRead);
    
        for (ma_uint64 frame = 0; frame < framesToRead; ++frame)
        {
            float allChannelsSum = 0.0f;

            for (ma_uint64 channel = 0; channel < decoder.outputChannels; ++channel)
            {
                const ma_uint64 sampleIndex = (samplingIndex * framesToRead * decoder.outputChannels) + (frame * decoder.outputChannels) + channel;
                const ma_uint64 multibandIndex = (frame * decoder.outputChannels) + channel;

                const float sampleValue     = frameData[sampleIndex];
                const float lowValue        = lowData[multibandIndex];
                const float midValue        = midData[multibandIndex];
                const float highValue       = highData[multibandIndex];

                allChannelsSum += std::abs(sampleValue) / decoder.outputChannels;

                result.globalFrames[samplingIndex].channelSumAverage += (std::abs(sampleValue) / decoder.outputChannels) / framesToRead;
                result.globalFrames[samplingIndex].lowAverage += (std::abs(lowValue) / decoder.outputChannels) / framesToRead;     
                result.globalFrames[samplingIndex].midAverage += (std::abs(midValue) / decoder.outputChannels) / framesToRead;    
                result.globalFrames[samplingIndex].highAverage += (std::abs(highValue) / decoder.outputChannels) / framesToRead;   

                result.channelFrames[channel][samplingIndex].min = std::min<float>(result.channelFrames[channel][samplingIndex].min, sampleValue);
                result.channelFrames[channel][samplingIndex].max = std::max<float>(result.channelFrames[channel][samplingIndex].max, sampleValue);
            }

            if (decoder.outputChannels == 2)
            {
                const ma_uint64 leftSampleIndex     = (samplingIndex * framesToRead * decoder.outputChannels) + (frame * decoder.outputChannels) + 0;
                const ma_uint64 rightSampleIndex    = (samplingIndex * framesToRead * decoder.outputChannels) + (frame * decoder.outputChannels) + 1;

                const float leftSampleValue = frameData[leftSampleIndex];
                const float rightSampleValue = frameData[rightSampleIndex];

                const float midValue = (leftSampleValue + rightSampleValue) / 2.0f;
                const float sideValue = (leftSampleValue - rightSampleValue) / 2.0f;

                result.stereoFrames[samplingIndex].midMin = std::min<float>(result.stereoFrames[samplingIndex].midMin, midValue);
                result.stereoFrames[samplingIndex].midMax = std::max<float>(result.stereoFrames[samplingIndex].midMax, midValue);
                result.stereoFrames[samplingIndex].sideMin = std::min<float>(result.stereoFrames[samplingIndex].sideMin, sideValue);
                result.stereoFrames[samplingIndex].sideMax = std::max<float>(result.stereoFrames[samplingIndex].sideMax, sideValue);
            }

            result.globalFrames[samplingIndex].rms                  += allChannelsSum * allChannelsSum;
        }

        result.globalFrames[samplingIndex].rms /= framesToRead;
    }

    ma_decoder_uninit(&decoder);
    ma_lpf_uninit(&lowsLowpass, NULL);
    ma_hpf_uninit(&highsHighpass, NULL);
    ma_lpf_uninit(&midsLowpass, NULL);
    ma_hpf_uninit(&midsHighpass, NULL);

    co_return result;
}

auto gluten::audio_subsystem::generate_sample_resolution_waveform(const std::filesystem::path filePath) -> concurrencpp::result<waveform>
{
    if (!std::filesystem::exists(filePath))
    {
        co_return waveform();
    }

    ma_decoder decoder;
    SC_ZERO_OBJECT(&decoder);
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, ma_standard_sample_rate_48000);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) != MA_SUCCESS)
    {
        co_return waveform();
    }

    ma_uint64 frameCount;
    ma_uint64 framesRead = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    waveform result(frameCount, decoder.outputChannels);
    
    std::vector<float> frameData(frameCount * decoder.outputChannels, 0.0f);

    ma_decoder_read_pcm_frames(&decoder, frameData.data(), frameCount, &framesRead);

    for (ma_uint64 frame = 0; frame < framesRead; ++frame)
    {
        for (ma_uint64 channel = 0; channel < decoder.outputChannels; ++channel)
        {
            const ma_uint64 sampleIndex = (frame * decoder.outputChannels) + channel;
            result.channelFrames[channel][frame].sample = frameData[sampleIndex];
        }
    }

    ma_decoder_uninit(&decoder);

    co_return result;
}
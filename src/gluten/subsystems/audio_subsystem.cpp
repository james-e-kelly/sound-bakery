#include "audio_subsystem.h"

#include "sound_bakery/runtime/runtime.h"

#include "ebur128/ebur128.h"
#include "gluten/app/app.h"
#include <array>
#include <numeric>

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

    sbk_system_config systemConfig = sbk_system_config_init_default();
    systemConfig.logToConsole            = true;
    systemConfig.soundChefConfig.outputChannels = m_outputChannels;
    result = sbk_system_init(systemConfig);
    if (result != SBK_SUCCESS)
    {
        return 1;
    }    

    return 0;
}

void gluten::audio_subsystem::tick(double deltaTime)
{
    (void)deltaTime;
    (void)sbk_system_update();

    release_finished_sounds();
}

void gluten::audio_subsystem::exit()
{
    m_pendingSoundReleases.clear();
    m_filesToVoiceHandlesMap.clear();
    m_filesToSoundsMap.clear();
    (void)sbk_system_destroy();
}

auto gluten::audio_subsystem::play_sound(const std::filesystem::path& filePath) -> void
{
    loop_data* loopData = get_sound_loop_info(filePath);

    const auto try_set_loop_points = [](sc_voice_handle handle, loop_data* loopData)
    {
        if (handle && loopData)
        {
            if (loopData->m_loopStart >= 0.0f && loopData->m_loopEnd >= 0.0f)
            {
                sc_voice_set_loop_position_in_seconds(sbk::engine::system::get()->get_runtime(), handle, loopData->m_loopStart, loopData->m_loopEnd);
            }
        }
    };

    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        sc_voice_set_paused(sbk::engine::system::get()->get_runtime(), voiceHandle, MA_FALSE);
        try_set_loop_points(voiceHandle, loopData);
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        // Starts paused: the voice can be mixed as soon as it exists, and a block at default volume and
        // channel mapping before its settings land is an audible click.
        sc_voice_handle handle{};
        sc_system_play_sound(sbk::engine::system::get()->get_runtime(), sound, &handle, nullptr, SC_TRUE);
        try_set_loop_points(handle, loopData);
        apply_voice_settings(filePath, handle);
        sc_voice_set_paused(sbk::engine::system::get()->get_runtime(), handle, SC_FALSE);
        m_filesToVoiceHandlesMap[filePath] = handle;
    }
}

auto gluten::audio_subsystem::pause_sound(const std::filesystem::path& filePath) -> void
{
    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        sc_voice_set_paused(sbk::engine::system::get()->get_runtime(), voiceHandle, MA_TRUE);
    }
}

auto gluten::audio_subsystem::pause_all() -> void
{
    for (const auto& soundInstance : m_filesToVoiceHandlesMap)
    {
        if (soundInstance.second)
        {
            sc_voice_set_paused(sbk::engine::system::get()->get_runtime(), soundInstance.second, MA_TRUE);
        }
    }
}

auto gluten::audio_subsystem::apply_to_all_voices(const std::function<void(sc_system*, sc_voice_handle)>& apply) -> void
{
    sc_system* const runtime = sbk::engine::system::get()->get_runtime();

    for (const auto& soundInstance : m_filesToVoiceHandlesMap)
    {
        if (soundInstance.second)
        {
            apply(runtime, soundInstance.second);
        }
    }
}

auto gluten::audio_subsystem::get_preview_volume() const -> float
{
    return m_previewMuted || m_previewVolumeDb <= s_previewVolumeMinDb ? 0.0f : std::pow(10.0f, m_previewVolumeDb / 20.0f);
}

auto gluten::audio_subsystem::set_preview_volume_db(float volumeDb) -> void
{
    m_previewVolumeDb = volumeDb;
    apply_to_all_voices([volume = get_preview_volume()](sc_system* runtime, sc_voice_handle voice) { (void)sc_voice_set_volume(runtime, voice, volume); });
}

auto gluten::audio_subsystem::set_preview_muted(bool muted) -> void
{
    m_previewMuted = muted;
    apply_to_all_voices([volume = get_preview_volume()](sc_system* runtime, sc_voice_handle voice) { (void)sc_voice_set_volume(runtime, voice, volume); });
}

auto gluten::audio_subsystem::set_preview_speed(float speed) -> void
{
    m_previewSpeed = std::clamp(speed, s_previewSpeedMin, s_previewSpeedMax);
    apply_to_all_voices([speed = m_previewSpeed](sc_system* runtime, sc_voice_handle voice) { (void)sc_voice_set_pitch(runtime, voice, speed); });
}

auto gluten::audio_subsystem::set_preview_looping(bool looping) -> void
{
    m_previewLooping = looping;
    apply_to_all_voices([looping](sc_system* runtime, sc_voice_handle voice) { (void)sc_voice_set_looping(runtime, voice, looping ? SC_TRUE : SC_FALSE); });
}

auto gluten::audio_subsystem::set_sound_cursor_position(const std::filesystem::path& filePath, float cursorPosition) -> void
{
    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        sc_voice_set_cursor_position_in_seconds(sbk::engine::system::get()->get_runtime(), voiceHandle, cursorPosition);
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_voice_handle handle{};
        sc_system_play_sound(sbk::engine::system::get()->get_runtime(), sound, &handle, nullptr, SC_TRUE);
        sc_voice_set_cursor_position_in_seconds(sbk::engine::system::get()->get_runtime(), handle, cursorPosition);
        apply_voice_settings(filePath, handle);
        m_filesToVoiceHandlesMap[filePath] = handle;
    }
}

auto gluten::audio_subsystem::set_sound_loop_start_position(const std::filesystem::path& filePath, float loopPosition) -> void
{
    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
        {
            sc_voice_set_looping(sbk::engine::system::get()->get_runtime(), voiceHandle, SC_FALSE);
        }

        loopData->m_loopStart = loopPosition;
    }
}

auto gluten::audio_subsystem::set_sound_loop_end_position(const std::filesystem::path& filePath, float loopPosition) -> void
{
    if (loop_data* loopData = get_sound_loop_info(filePath))
    {
        if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
        {
            sc_voice_set_looping(sbk::engine::system::get()->get_runtime(), voiceHandle, SC_TRUE);
        }

        loopData->m_loopEnd = loopPosition;
    }
}

auto gluten::audio_subsystem::get_sound_cursor_position(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        (void)sc_voice_get_cursor_position_in_seconds(sbk::engine::system::get()->get_runtime(), voiceHandle, &seconds);
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

auto gluten::audio_subsystem::get_sound_sample_rate(const std::filesystem::path& filePath) -> std::uint32_t
{
    ZoneScoped;

    ma_uint32 sampleRate = 0;

    if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        ma_data_source_get_data_format(sound->dataSource, nullptr, nullptr, &sampleRate, nullptr, 0);
    }

    return sampleRate;
}

auto gluten::audio_subsystem::get_sound_is_playing(const std::filesystem::path& filePath) -> bool
{
    sc_bool playing = MA_FALSE;
    sc_bool paused  = MA_FALSE;

    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        (void)sc_voice_get_is_playing(sbk::engine::system::get()->get_runtime(), voiceHandle, &playing);
        (void)sc_voice_get_paused(sbk::engine::system::get()->get_runtime(), voiceHandle, &paused);
    }

    return playing && !paused;
}

auto gluten::audio_subsystem::get_sound_is_looping(const std::filesystem::path& filePath) -> bool
{
    sc_bool looping = MA_FALSE;

    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        (void)sc_voice_get_looping(sbk::engine::system::get()->get_runtime(), voiceHandle, &looping);
    }

    return looping;
}

auto gluten::audio_subsystem::stop_all_sounds() -> void
{
    for (const auto& soundInstance : m_filesToVoiceHandlesMap)
    {
        (void)sc_voice_stop(sbk::engine::system::get()->get_runtime(), soundInstance.second, 0.0F, sc_voice_stop_mode_cut_tail);
    }

    m_filesToVoiceHandlesMap.clear();
}

auto gluten::audio_subsystem::release_file(const std::filesystem::path& filePath) -> void
{
    sc_voice_handle voiceHandle{};

    if (const auto foundVoice = m_filesToVoiceHandlesMap.find(filePath); foundVoice != m_filesToVoiceHandlesMap.end())
    {
        voiceHandle = foundVoice->second;
        (void)sc_voice_stop(sbk::engine::system::get()->get_runtime(), voiceHandle, 0.0F, sc_voice_stop_mode_cut_tail);
        m_filesToVoiceHandlesMap.erase(foundVoice);
    }

    if (const auto foundSound = m_filesToSoundsMap.find(filePath); foundSound != m_filesToSoundsMap.end())
    {
        m_pendingSoundReleases.push_back({std::move(foundSound->second), voiceHandle});
        m_filesToSoundsMap.erase(foundSound);
    }

    release_finished_sounds();
    m_filesToLoopDataMap.erase(filePath);
    m_filesToMixMatrix.erase(filePath);
    m_filesToRouting.erase(filePath);
    m_filesToWaveforms.erase(filePath);
    m_waveformLodCache.erase(filePath);
}

auto gluten::audio_subsystem::get_output_channel_map() -> const std::vector<ma_channel>&
{
    if (!m_outputChannelMap.has_value())
    {
        sc_system* const system      = sbk::engine::system::get()->get_runtime();
        ma_engine* const engine      = &system->engine;
        const std::uint32_t channels = ma_engine_get_channels(engine);

        std::vector<ma_channel> channelMap(channels, MA_CHANNEL_NONE);

        if (const ma_device* const device = ma_engine_get_device(engine); device && device->playback.channels == channels)
        {
            std::copy_n(device->playback.channelMap, channels, channelMap.begin());
        }
        else
        {
            ma_channel_map_init_standard(ma_standard_channel_map_default, channelMap.data(), channelMap.size(), channels);
        }

        std::array<char, SC_CHANNEL_LAYOUT_NAME_SIZE> description{};
        if (sc_channel_map_describe(channelMap.data(), channels, description.data(), description.size()) == SBK_SUCCESS)
        {
            m_outputDescription = description.data();
        }
        else
        {
            m_outputDescription = fmt::format("{} channels", channels);
        }

        m_outputChannelMap = std::move(channelMap);
        gluten::app::get()->get_logger()->info("Audio output: {} ({} channels)", m_outputDescription, m_outputChannelMap->size());
    }

    return *m_outputChannelMap;
}

auto gluten::audio_subsystem::get_output_description() -> const std::string&
{
    (void)get_output_channel_map();
    return m_outputDescription;
}

auto gluten::audio_subsystem::get_file_routing(const std::filesystem::path& filePath) -> const file_routing*
{
    if (const auto found = m_filesToRouting.find(filePath); found != m_filesToRouting.end())
    {
        return &found->second;
    }

    sc_sound* const sound = get_or_load_audio_handle(filePath);
    if (!sound)
    {
        return nullptr;
    }

    ma_uint32 channels = 0;
    if (ma_data_source_get_data_format(sound->dataSource, nullptr, &channels, nullptr, nullptr, 0) != MA_SUCCESS || channels == 0)
    {
        return nullptr;
    }

    file_routing& routing = m_filesToRouting[filePath];
    routing.channelCount  = channels;
    (void)sc_channel_layout_detect(nullptr, filePath.string().c_str(), channels, &routing.detectedLayout, &routing.detectedReason);
    routing.channelOrder.resize(channels);
    std::iota(routing.channelOrder.begin(), routing.channelOrder.end(), sc_uint32{0});
    routing.soloedChannels.assign(channels, SC_FALSE);

    gluten::app::get()->get_logger()->info("'{}': {} channels as {} ({}), playing to {}", filePath.filename().string(), channels, get_active_layout(routing).name, routing.detectedReason, get_output_description());

    update_routing(filePath, routing);
    return &routing;
}

auto gluten::audio_subsystem::get_active_layout(const file_routing& routing) const -> const sc_channel_layout&
{
    if (const auto found = m_layoutOverrides.find(routing.channelCount); found != m_layoutOverrides.end())
    {
        return found->second;
    }
    return routing.detectedLayout;
}

auto gluten::audio_subsystem::set_layout_override(std::size_t channelCount, std::optional<sc_channel_layout> layout) -> void
{
    if (layout.has_value())
    {
        m_layoutOverrides[channelCount] = std::move(*layout);
    }
    else
    {
        m_layoutOverrides.erase(channelCount);
    }

    for (auto& [filePath, routing] : m_filesToRouting)
    {
        if (routing.channelCount == channelCount)
        {
            update_routing(filePath, routing);
        }
    }
}

auto gluten::audio_subsystem::set_channel_source(const std::filesystem::path& filePath, std::size_t slot, std::size_t fileChannel) -> void
{
    const auto found = m_filesToRouting.find(filePath);
    if (found == m_filesToRouting.end() || slot >= found->second.channelCount || fileChannel >= found->second.channelCount)
    {
        return;
    }

    std::vector<sc_uint32>& order = found->second.channelOrder;
    std::iter_swap(order.begin() + static_cast<std::ptrdiff_t>(slot), std::find(order.begin(), order.end(), static_cast<sc_uint32>(fileChannel)));
    update_routing(filePath, found->second);
}

auto gluten::audio_subsystem::set_channel_soloed(const std::filesystem::path& filePath, std::size_t fileChannel, bool soloed) -> void
{
    const auto found = m_filesToRouting.find(filePath);
    if (found == m_filesToRouting.end() || fileChannel >= found->second.channelCount)
    {
        return;
    }

    found->second.soloedChannels[fileChannel] = soloed ? SC_TRUE : SC_FALSE;
    update_routing(filePath, found->second);
}

auto gluten::audio_subsystem::update_routing(const std::filesystem::path& filePath, file_routing& routing) -> void
{
    const sc_channel_layout& layout                 = get_active_layout(routing);
    const std::vector<ma_channel>& outputChannelMap = get_output_channel_map();
    const auto outputChannels                       = static_cast<sc_uint32>(outputChannelMap.size());

    routing.channelLabels.assign(routing.channelCount, std::string());
    for (std::size_t slot = 0; slot < routing.channelCount; ++slot)
    {
        std::array<char, SC_CHANNEL_LABEL_SIZE> label{};
        (void)sc_channel_layout_get_channel_label(&layout, static_cast<sc_uint32>(slot), label.data(), label.size());
        routing.channelLabels[routing.channelOrder[slot]] = label.data();
    }

    mix_matrix matrix{std::vector<float>(routing.channelCount * outputChannels), static_cast<std::uint32_t>(routing.channelCount), outputChannels};
    if (routing.channelCount <= SC_MAX_CHANNELS && outputChannels <= SC_MAX_CHANNELS && sc_channel_layout_build_mix_matrix(&layout, routing.channelOrder.data(), routing.soloedChannels.data(), outputChannelMap.data(), outputChannels, matrix.gains.data()) == SBK_SUCCESS)
    {
        m_filesToMixMatrix[filePath] = std::move(matrix);
    }
    else
    {
        gluten::app::get()->get_logger()->warn("Routing supports up to {} channels in and out; '{}' uses the default mapping for {} in, {} out", static_cast<int>(SC_MAX_CHANNELS), filePath.filename().string(), routing.channelCount, outputChannels);
        m_filesToMixMatrix.erase(filePath);
    }

    if (sc_voice_handle voiceHandle = get_voice_handle(filePath))
    {
        apply_voice_settings(filePath, voiceHandle);
    }
}

auto gluten::audio_subsystem::apply_voice_settings(const std::filesystem::path& filePath, sc_voice_handle voiceHandle) -> void
{
    sc_system* const runtime = sbk::engine::system::get()->get_runtime();

    (void)get_file_routing(filePath);

    (void)sc_voice_set_volume(runtime, voiceHandle, get_preview_volume());
    (void)sc_voice_set_pitch(runtime, voiceHandle, m_previewSpeed);
    (void)sc_voice_set_looping(runtime, voiceHandle, m_previewLooping ? SC_TRUE : SC_FALSE);

    if (const auto found = m_filesToMixMatrix.find(filePath); found != m_filesToMixMatrix.end())
    {
        (void)sc_voice_set_mix_matrix(runtime, voiceHandle, found->second.gains.data(), found->second.channelsIn, found->second.channelsOut);
    }
    else
    {
        (void)sc_voice_set_mix_matrix(runtime, voiceHandle, nullptr, 0, 0);
    }
}

auto gluten::audio_subsystem::release_finished_sounds() -> void
{
    std::erase_if(m_pendingSoundReleases,
                  [](const pending_sound_release& pending)
                  {
                      sc_bool playing = MA_FALSE;
                      return !pending.voiceHandle || sc_voice_get_is_playing(sbk::engine::system::get()->get_runtime(), pending.voiceHandle, &playing) != SBK_SUCCESS;
                  });
}

auto gluten::audio_subsystem::get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*
{
    ZoneScoped;

    if (const auto foundSound = m_filesToSoundsMap.find(filePath); foundSound != m_filesToSoundsMap.end())
    {
        return foundSound->second.get();
    }

    sc_sound* sound = nullptr;

    if (std::filesystem::exists(filePath))
    {
        const std::string path = filePath.string();
        const sc_sound_config config = sc_sound_config_init_file(path.c_str(), m_soundLoadMode);
        if (sc_system_create_sound(sbk::engine::system::get()->get_runtime(), &config, &sound) == SBK_SUCCESS)
        {
            m_filesToSoundsMap[filePath].reset(sound);
        }
    }
    return sound;
}

auto gluten::audio_subsystem::get_voice_handle(const std::filesystem::path& filePath) -> sc_voice_handle
{
    sc_voice_handle voiceHandle{};

    if (const auto foundIter = m_filesToVoiceHandlesMap.find(filePath); foundIter != m_filesToVoiceHandlesMap.end())
    {
        // Voices are freed when they finish, leaving the stored handle stale.
        sc_bool playing = MA_FALSE;
        if (sc_voice_get_is_playing(sbk::engine::system::get()->get_runtime(), foundIter->second, &playing) == SBK_SUCCESS)
        {
            voiceHandle = foundIter->second;
        }
        else
        {
            m_filesToVoiceHandlesMap.erase(foundIter);
        }
    }
    return voiceHandle;
}

auto gluten::audio_subsystem::get_ui_waveform_lods(const std::filesystem::path& filePath) -> waveform_lods_cache_type::cache_result
{
    ZoneScoped;

    if (m_waveformLodCache.get_cache_needs_filling(filePath))
    {
        m_waveformLodCache.set_async_fill_cache(filePath, async_generate_waveform_lods(filePath, 0));
    }

    return m_waveformLodCache.get_cached_data(filePath);
}

auto gluten::audio_subsystem::get_sound_loop_info(const std::filesystem::path& filePath) -> loop_data*
{
    if (const auto foundLoop = m_filesToLoopDataMap.find(filePath); foundLoop != m_filesToLoopDataMap.end())
    {
        return &foundLoop->second;
    }

    if (std::filesystem::exists(filePath))
    {
        return &m_filesToLoopDataMap.emplace(filePath, loop_data()).first->second;
    }
    return nullptr;
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

auto gluten::audio_subsystem::async_generate_waveform_lods(const std::filesystem::path filePath, std::size_t resolution) -> concurrencpp::result<waveform_lods>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    waveform_lods result;

    ma_decoder decoder;
    SC_ZERO_OBJECT(&decoder);

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, ma_standard_sample_rate_48000);
    sc_decoder_config_add_custom_backends(&config);

    if (ma_decoder_init_file(filePath.string().c_str(), &config, &decoder) == MA_SUCCESS)
    {
        ma_uint64 frameCount;
        ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

        std::shared_ptr<const std::vector<float>> frameData = std::make_shared<const std::vector<float>>(frameCount * decoder.outputChannels, 0.0f);
        ma_decoder_read_pcm_frames(&decoder, const_cast<float*>(frameData->data()), frameCount, nullptr);
        const ma_uint32 channels  = decoder.outputChannels;
        const double fileDuration = static_cast<double>(frameCount) / ma_standard_sample_rate_48000;
        ma_decoder_uninit(&decoder);

        if (frameCount == 0)
        {
            gluten::app::get()->get_logger()->warn("No audio decoded for the waveform of '{}'", filePath.string());
        }

        concurrencpp::shared_result<waveform_lod> thumbnailResult = async_generate_waveform_lod(frameData, channels, fileDuration, 10, {});

        result.thumbnailRes.set_async_fill_cache(thumbnailResult);
        result.lowRes.set_async_fill_cache(async_generate_waveform_lod(frameData, channels, fileDuration, 100, thumbnailResult));
        result.medRes.set_async_fill_cache(async_generate_waveform_lod(frameData, channels, fileDuration, 1000, thumbnailResult));
        result.highRes.set_async_fill_cache(async_generate_waveform_lod(frameData, channels, fileDuration, 3000, thumbnailResult));
        result.sampleRes.set_async_fill_cache(async_generate_waveform_lod(frameData, channels, fileDuration, 48000, thumbnailResult));
    }
    else
    {
        gluten::app::get()->get_logger()->warn("Couldn't decode '{}' for the waveform", filePath.string());
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
                result.channelFrames[channel][samplingIndex].rms += sampleValue * sampleValue;
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

        for (ma_uint64 channel = 0; channel < channels; ++channel)
        {
            float& rms = result.channelFrames[channel][samplingIndex].rms;
            rms        = std::sqrt(rms * framesToReadReciprocal);
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

    ebur128_state* eburState = ebur128_init(channels, ma_standard_sample_rate_48000, EBUR128_MODE_M | EBUR128_MODE_S | EBUR128_MODE_TRUE_PEAK);

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

            {
                ZoneScopedN("true_peak");
                for (ma_uint32 channel = 0; channel < channels; ++channel)
                {
                    double channelTruePeak = 0.0;
                    if (ebur128_prev_true_peak(eburState, channel, &channelTruePeak) == EBUR128_SUCCESS)
                    {
                        result[samplingIndex].truePeak = std::max(result[samplingIndex].truePeak, static_cast<float>(channelTruePeak));
                    }
                }
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

        ebur128_state* eburState = ebur128_init(channels, ma_standard_sample_rate_48000, EBUR128_MODE_I | EBUR128_MODE_LRA | EBUR128_MODE_HISTOGRAM | EBUR128_MODE_TRUE_PEAK);

        ebur128_add_frames_float(eburState, audioData->data(), frameCount);
        ebur128_loudness_global(eburState, &result.lufs.integrated);
        ebur128_loudness_range(eburState, &result.lufs.range);

        double maxTruePeak = 0.0;
        for (ma_uint32 channel = 0; channel < channels; ++channel)
        {
            double channelTruePeak = 0.0;
            if (ebur128_true_peak(eburState, channel, &channelTruePeak) == EBUR128_SUCCESS)
            {
                maxTruePeak = std::max(maxTruePeak, channelTruePeak);
            }
        }
        result.lufs.truePeak = maxTruePeak > 0.0 ? 20.0 * std::log10(maxTruePeak) : -200.0;

        ebur128_destroy(&eburState);
    }

    co_return result;
}
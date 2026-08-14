#pragma once

#include "sound_bakery/system.h"

#include "concurrencpp/concurrencpp.h"
#include "gluten/data/data_cache.h"
#include "gluten/subsystems/subsystem.h"

namespace gluten
{
    struct loudness_lufs
    {
        double shortterm = -200.0;
        double momentary = -200.0;
    };

    struct loudness_lufs_global
    {
        double integrated = -200.0;
        double range      = -200.0;
    };

    /**
     * @brief Min/max of the channel
     */
    struct channel_frame
    {
        float min    = 0.0f;  // min sample value within the time frame. only written to when downsampling
        float max    = 0.0f;  // max sample value within the time frame. only written to when downsampling
        float sample = 0.0f;  // the actual sample value at this frame. only written to when showing the full resolution
    };

    /**
     * @brief Mid/side information
     */
    struct stereo_data
    {
        float midMin  = 0.0f;
        float midMax  = 0.0f;
        float sideMin = 0.0f;
        float sideMax = 0.0f;
    };

    /**
     * @brief Data about the frame as a whole, like overall volumes
     */
    struct frame_data
    {
        float rms               = 0.0f;
        float channelSumAverage = 0.0f;
        float lowAverage        = 0.0f;
        float midAverage        = 0.0f;
        float highAverage       = 0.0f;

        loudness_lufs lufs;
    };

    struct waveform
    {
        using global_frame_cache_type = single_data_cache<std::vector<frame_data>>;

        waveform() = default;
        waveform(std::size_t frames, std::size_t channels)
            : channelFrames(channels, std::vector<channel_frame>(frames, channel_frame()))
        {
            if (channels == 2)
            {
                stereoFrames.assign(frames, stereo_data());
            }
        }

        std::vector<std::vector<channel_frame>> channelFrames;  // indexed [channel][frame] so each channel can be sent to ImPlot asap
        std::vector<stereo_data> stereoFrames;                  // Only valid when there are only two channels
        global_frame_cache_type globalFramesCache;              // Total volume sums

        loudness_lufs_global lufs;

        auto is_stereo() const -> bool
        {
            return channelFrames.size() == 2;
        }
    };

    struct waveform_lod
    {
        int resolution;
        waveform lodWaveform;
    };

    /**
     * @brief A simple audio subsystem that can play audio using Sound Bakery.
     */
    class audio_subsystem : public subsystem
    {
    public:
        using loudness_cache_type     = data_cache<loudness_lufs, key_cache_key<std::filesystem::path>, key_cache_key_hasher<std::filesystem::path>>;
        using waveform_lod_cache_type = single_data_cache<waveform_lod>;

        struct waveform_lods
        {
            waveform_lod_cache_type thumbnailRes;
            waveform_lod_cache_type lowRes;
            waveform_lod_cache_type medRes;
            waveform_lod_cache_type highRes;
            waveform_lod_cache_type sampleRes;
        };

        using waveform_lods_cache_type = data_cache<waveform_lods, key_cache_key<std::filesystem::path>, key_cache_key_hasher<std::filesystem::path>>;

        audio_subsystem(app* appOwner) : subsystem(appOwner) {}

        int init() override;
        void exit() override;

        auto play_sound(const std::filesystem::path& filePath) -> void;
        auto pause_sound(const std::filesystem::path& filePath) -> void;
        auto pause_all() -> void;
        auto set_sound_cursor_position(const std::filesystem::path& filePath, float cursorPosition) -> void;
        auto set_sound_loop_start_position(const std::filesystem::path& filePath, float loopPosition) -> void;
        auto set_sound_loop_end_position(const std::filesystem::path& filePath, float loopPosition) -> void;
        auto get_sound_cursor_position(const std::filesystem::path& filePath) -> float;
        auto get_sound_loop_start_position(const std::filesystem::path& filePath) -> float;
        auto get_sound_loop_end_position(const std::filesystem::path& filePath) -> float;
        auto get_sound_length(const std::filesystem::path& filePath) -> float;
        auto get_sound_is_playing(const std::filesystem::path& filePath) -> bool;
        auto get_sound_is_looping(const std::filesystem::path& filePath) -> bool;
        auto stop_all_sounds() -> void;

        auto get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*;
        auto get_sound_instance(const std::filesystem::path& filePath) -> sc_sound_instance*;

        auto get_ui_waveform_lods(const std::filesystem::path& filePath, double fileDuration) -> waveform_lods_cache_type::cache_result;

    protected:
        struct sc_sound_deleter
        {
            void operator()(sc_sound* sound) { sc_sound_release(sound); }
        };

        struct sc_sound_instance_deleter
        {
            void operator()(sc_sound_instance* soundInstance) { sc_sound_instance_release(soundInstance); }
        };

        struct loop_data
        {
            float m_loopStart = -1.0f;
            float m_loopEnd   = -1.0f;
        };

        auto get_sound_loop_info(const std::filesystem::path& filePath) -> loop_data*;

        auto async_generate_waveform_lod(std::shared_ptr<const std::vector<float>> audioData, ma_uint64 channels, double fileDuration, std::size_t resolution, concurrencpp::shared_result<waveform_lod> dependencyResult) -> concurrencpp::result<waveform_lod>;
        auto async_generate_waveform_lods(const std::filesystem::path filePath, double fileDuration, std::size_t resolution) -> concurrencpp::result<waveform_lods>;
        auto generate_downsampled_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<waveform>;
        auto generate_downsampled_resolution_global_frames(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<std::vector<frame_data>>;
        auto generate_sample_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, ma_uint32 channels) -> concurrencpp::result<waveform>;

        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound, sc_sound_deleter>> m_filesToSoundsMap;
        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound_instance, sc_sound_deleter>> m_filesToSoundInstancesMap;
        std::unordered_map<std::filesystem::path, loop_data> m_filesToLoopDataMap;

        std::unordered_map<std::filesystem::path, waveform> m_filesToWaveforms;
        waveform_lods_cache_type m_waveformLodCache;
        loudness_cache_type m_filesToLoudnessCache;
    };
}  // namespace gluten
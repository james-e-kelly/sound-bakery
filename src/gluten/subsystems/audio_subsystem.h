#pragma once

#include "gluten/subsystems/subsystem.h"

#include "gluten/data/data_cache.h"
#include "concurrencpp/concurrencpp.h"
#include "sound_bakery/system.h"

namespace gluten
{
    struct loudness_lufs
    {
        double integrated = -200.0;
        double shorttermMax = -200.0;
        double momentaryMax = -200.0;
    };

    /**
     * @brief Volume data about one bucket and one channel
     */
    struct channel_bucket
    {
        float min;
        float max;
        float rms;
        float smoothedRms;
        float averageSample;
    };

	/**
	 * @brief A simple audio subsystem that can play audio using Sound Bakery.
	 */
	class audio_subsystem : public subsystem
	{
    public:
        using waveform_frame = std::vector<channel_bucket>;                //< All channels in the bucket
        using waveform = std::vector<waveform_frame>;                      //< All buckets and all channels
        using waveform_generator = concurrencpp::generator<waveform_frame>;
        using loudness_cache_type = data_cache<loudness_lufs, key_cache_key<std::filesystem::path>, key_cache_key_hasher<std::filesystem::path>>;

        struct waveform_lod
        {
            int resolution;
            waveform waveform;
        };

        using waveform_lod_cache_type = data_cache<waveform_lod, key_cache_key<std::filesystem::path>, key_cache_key_hasher<std::filesystem::path>>;

        struct waveform_lods
        {
            waveform_lod_cache_type thumbnailRes;
            waveform_lod_cache_type lowRes;
            waveform_lod_cache_type medRes;
            waveform_lod_cache_type highRes;
            waveform_lod_cache_type sampleRes;
        };

        audio_subsystem(app* appOwner) : subsystem(appOwner) {}

        int init() override;

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

        auto get_ui_waveform(const std::filesystem::path& filePath, std::size_t buckets) -> waveform&;
        auto get_ui_waveform_lods(const std::filesystem::path& filePath, double fileDuration) -> waveform_lods&;

        auto get_loudness_lufs(const std::filesystem::path& filePath) -> loudness_cache_type::cache_result;

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
            float m_loopEnd = -1.0f;
        };

        auto get_sound_loop_info(const std::filesystem::path& filePath) -> loop_data*;

        auto async_generate_waveform(const std::filesystem::path filePath, std::size_t targetSamples) -> concurrencpp::result<void>;
        auto async_generate_waveform_lod(const std::filesystem::path filePath, double fileDuration, std::size_t resolution) -> concurrencpp::result<waveform_lod>;
        auto async_calculate_loudness(const std::filesystem::path filePath) -> loudness_cache_type::async_cache_result;
        auto generate_waveform(const std::filesystem::path filePath, std::size_t targetSamples) -> waveform_generator;

        std::unique_ptr<sbk::engine::system> m_soundBakery;
        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound, sc_sound_deleter>> m_filesToSoundsMap;
        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound_instance, sc_sound_deleter>> m_filesToSoundInstancesMap;
        std::unordered_map<std::filesystem::path, loop_data> m_filesToLoopDataMap;

        std::unordered_map<std::filesystem::path, waveform> m_filesToWaveforms;
        std::unordered_map<std::filesystem::path, waveform_lods> m_filesToWaveformLods;
        loudness_cache_type m_filesToLoudnessCache; 
	};
}
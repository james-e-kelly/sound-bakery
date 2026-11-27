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
        double truePeak   = -200.0;  //< dBTP, loudest inter-sample peak across all channels
    };

    /**
     * @brief Min/max of the channel
     */
    struct channel_frame
    {
        float min    = 0.0f;  // min sample value within the time frame. only written to when downsampling
        float max    = 0.0f;  // max sample value within the time frame. only written to when downsampling
        float sample = 0.0f;  // the actual sample value at this frame. only written to when showing the full resolution
        float rms    = 0.0f;  // RMS of the channel within the time frame. only written to when downsampling
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
        float truePeak          = 0.0f;  //< Linear, loudest channel. Only written at the LUFS resolution

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
        void tick(double deltaTime) override;
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
        auto get_sound_sample_rate(const std::filesystem::path& filePath) -> std::uint32_t;  //< The file's own rate, before any resampling. 0 if it can't be loaded
        auto get_sound_is_playing(const std::filesystem::path& filePath) -> bool;
        auto get_sound_is_looping(const std::filesystem::path& filePath) -> bool;
        auto stop_all_sounds() -> void;

        /**
         * @brief Stops the file and frees its sound, voice, loop points and waveform data.
         *
         * Nothing is freed automatically, so apps that pass through many files (like a file browser) should
         * release the ones they move away from. Sample-resolution waveforms are several times the file's size.
         */
        auto release_file(const std::filesystem::path& filePath) -> void;

        /**
         * @brief How sounds loaded after this call are read. Defaults to decoding the whole file up front;
         * streaming starts playback quicker on large or remote files.
         */
        auto set_sound_load_mode(sc_sound_mode mode) -> void { m_soundLoadMode = mode; }

        /**
         * @brief Channels to mix to. Must be set before init; 0 (the default) uses the output device's own channel count,
         * so surround and ambisonic files can be routed to every speaker.
         */
        auto set_output_channel_count(std::uint32_t channels) -> void { m_outputChannels = channels; }

        /**
         * @brief The speaker position of each output channel.
         */
        auto get_output_channel_map() -> const std::vector<ma_channel>&;
        auto get_output_description() -> const std::string&;  //< e.g. "5.1" or "Stereo"

        /**
         * @brief How a file's channels are laid out and routed to the output.
         */
        struct file_routing
        {
            std::size_t channelCount = 0;
            sc_channel_layout detectedLayout{};
            const char* detectedReason = "";         //< How the layout was decided, e.g. "WAV channel mask"
            std::vector<sc_uint32> channelOrder;     //< For each layout slot, the file channel feeding it
            std::vector<sc_bool> soloedChannels;     //< Per file channel
            std::vector<std::string> channelLabels;  //< Per file channel, e.g. "Ls" or "W"
        };

        /**
         * @brief The file's routing, detecting its layout the first time. Null if the file can't be loaded.
         */
        auto get_file_routing(const std::filesystem::path& filePath) -> const file_routing*;
        auto get_active_layout(const file_routing& routing) const -> const sc_channel_layout&;

        /**
         * @brief Uses a layout for every file with this many channels, instead of the detected one. Empty goes back to detection.
         */
        auto set_layout_override(std::size_t channelCount, std::optional<sc_channel_layout> layout) -> void;
        auto has_layout_override(std::size_t channelCount) const -> bool { return m_layoutOverrides.contains(channelCount); }

        /**
         * @brief Feeds a layout slot from a different file channel, swapping with the slot that had it.
         */
        auto set_channel_source(const std::filesystem::path& filePath, std::size_t slot, std::size_t fileChannel) -> void;
        auto set_channel_soloed(const std::filesystem::path& filePath, std::size_t fileChannel, bool soloed) -> void;

        /**
         * @brief Preview controls shared by every voice this subsystem plays. Previews start quiet so nobody
         * gets blasted by a loud master on first play.
         */
        auto set_preview_volume_db(float volumeDb) -> void;
        auto get_preview_volume_db() const -> float { return m_previewVolumeDb; }
        auto set_preview_muted(bool muted) -> void;
        auto get_preview_muted() const -> bool { return m_previewMuted; }
        auto get_preview_volume() const -> float;  //< Linear gain after mute

        /**
         * @brief Varispeed: playback rate, changing pitch with it like tape.
         */
        auto set_preview_speed(float speed) -> void;
        auto get_preview_speed() const -> float { return m_previewSpeed; }

        /**
         * @brief Loops whole files back to the start when they reach the end.
         */
        auto set_preview_looping(bool looping) -> void;
        auto get_preview_looping() const -> bool { return m_previewLooping; }

        static constexpr float s_previewVolumeMinDb = -60.0f;  //< At or below this, the preview is silent
        static constexpr float s_previewSpeedMin    = 0.05f;
        static constexpr float s_previewSpeedMax    = 2.0f;

        auto get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*;
        auto get_voice_handle(const std::filesystem::path& filePath) -> sc_voice_handle;

        auto get_ui_waveform_lods(const std::filesystem::path& filePath) -> waveform_lods_cache_type::cache_result;

    protected:
        struct sc_sound_deleter
        {
            void operator()(sc_sound* sound) { sc_sound_release(sound); }
        };

        struct loop_data
        {
            float m_loopStart = -1.0f;
            float m_loopEnd   = -1.0f;
        };

        /**
         * @brief A released sound waits here until its voice has actually stopped, as the mixer may still be reading it.
         */
        struct pending_sound_release
        {
            std::unique_ptr<sc_sound, sc_sound_deleter> sound;
            sc_voice_handle voiceHandle{};
        };

        struct mix_matrix
        {
            std::vector<float> gains;
            std::uint32_t channelsIn  = 0;
            std::uint32_t channelsOut = 0;
        };

        auto get_sound_loop_info(const std::filesystem::path& filePath) -> loop_data*;
        auto release_finished_sounds() -> void;
        auto apply_voice_settings(const std::filesystem::path& filePath, sc_voice_handle voiceHandle) -> void;
        auto update_routing(const std::filesystem::path& filePath, file_routing& routing) -> void;
        auto apply_to_all_voices(const std::function<void(sc_system*, sc_voice_handle)>& apply) -> void;

        auto async_generate_waveform_lod(std::shared_ptr<const std::vector<float>> audioData, ma_uint64 channels, double fileDuration, std::size_t resolution, concurrencpp::shared_result<waveform_lod> dependencyResult) -> concurrencpp::result<waveform_lod>;
        auto async_generate_waveform_lods(const std::filesystem::path filePath, std::size_t resolution) -> concurrencpp::result<waveform_lods>;
        auto generate_downsampled_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<waveform>;
        auto generate_downsampled_resolution_global_frames(std::shared_ptr<const std::vector<float>> audioData, std::size_t resolution, ma_uint32 channels, std::size_t targetSamples) -> concurrencpp::result<std::vector<frame_data>>;
        auto generate_sample_resolution_waveform(std::shared_ptr<const std::vector<float>> audioData, ma_uint32 channels) -> concurrencpp::result<waveform>;

        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound, sc_sound_deleter>> m_filesToSoundsMap;
        std::unordered_map<std::filesystem::path, sc_voice_handle> m_filesToVoiceHandlesMap;
        std::unordered_map<std::filesystem::path, loop_data> m_filesToLoopDataMap;
        std::vector<pending_sound_release> m_pendingSoundReleases;
        std::unordered_map<std::filesystem::path, mix_matrix> m_filesToMixMatrix;
        std::unordered_map<std::filesystem::path, file_routing> m_filesToRouting;
        std::map<std::size_t, sc_channel_layout> m_layoutOverrides;
        std::optional<std::vector<ma_channel>> m_outputChannelMap;
        std::string m_outputDescription;

        std::unordered_map<std::filesystem::path, waveform> m_filesToWaveforms;
        waveform_lods_cache_type m_waveformLodCache;
        loudness_cache_type m_filesToLoudnessCache;

        float m_previewVolumeDb = 0.0f;
        bool m_previewMuted     = false;
        float m_previewSpeed    = 1.0f;
        bool m_previewLooping   = false;

        sc_sound_mode m_soundLoadMode = SC_SOUND_MODE_DECODE;
        std::uint32_t m_outputChannels = 0;
    };
}  // namespace gluten
#pragma once

#include "gluten/theme/theme.h"

namespace gluten
{
    class audio_element;

    /**
     * @brief A flat, square-cornered control button. Neighbouring buttons overlap by a pixel and round only their
     * outer corners, so a run of them reads as one segmented control.
     */
    auto transport_button(const char* id,
                          const ImRect& rect,
                          const char* label,
                          ImFont* font,
                          float fontSize,
                          bool active          = false,
                          ImDrawFlags corners  = ImDrawFlags_RoundCornersAll,
                          bool enabled         = true,
                          const ImVec4& activeFill = theme::interactive,
                          const ImVec4& activeText = theme::textOnColor) -> bool;

    /**
     * @brief The preview speed slider and readout. Speed lives in the audio subsystem, so any transport showing it
     * (audio or video) controls the same value. Ends at right; returns where it starts.
     */
    auto render_preview_speed(float right, float centerY) -> float;
    auto get_preview_speed_width() -> float;

    /**
     * @brief Mute, preview volume fader and readout, shared the same way as the speed. Ends at right; returns where
     * they start.
     */
    auto render_preview_volume(float right, float top, float centerY) -> float;
    auto get_preview_volume_width() -> float;

    /**
     * @brief App-specific controls drawn into the transport between the timecode and the shared controls.
     */
    struct transport_extras
    {
        float width = 0.0f;
        std::function<void(const ImRect& area)> render;
    };

    /**
     * @brief The transport every Gluten audio app shares: play/stop, timecode, waveform overlay toggles,
     * speed, mute, volume and an output meter. Volume, mute and speed live in the audio subsystem, so
     * they carry across files.
     *
     * Wraps onto a second row when the width can't fit everything on one.
     */
    class audio_transport
    {
    public:
        static auto get_height(float width, float extrasWidth = 0.0f) -> float;
        static auto render(const ImRect& area, audio_element& waveform, const transport_extras& extras = {}) -> void;

        static constexpr float s_rowHeight  = 36.0f;
        static constexpr float s_buttonSize = 28.0f;
    };

    /**
     * @brief Layout picker and per-channel routing (reorder, solo) for files with more than two channels.
     * Takes no space for mono and stereo files.
     */
    class audio_routing_controls
    {
    public:
        static auto get_height(const std::filesystem::path& filePath, float width) -> float;
        static auto render(const ImRect& area, const std::filesystem::path& filePath) -> void;

        static constexpr float s_barHeight     = 28.0f;
        static constexpr float s_gridRowHeight = 26.0f;
    };
}  // namespace gluten
